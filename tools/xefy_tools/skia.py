"""Skia build orchestration.

Layout: third_party/skia/out/<target>/ holds the GN output dir. Xefy adds a stamp
file `.xefy-stamp.json` recording the pinned revision and the sha256 of
tools/skia_args/<target>.gn, so `setup` can skip builds that are up to date.
"""
import os
import sys

from . import host as hostmod
from .util import (ROOT, TOOLS_DIR, ToolError, info, out_of, read_json, rel, run,
                   sha256_file, write_json)

ARGS_DIR = os.path.join(TOOLS_DIR, "skia_args")
STAMP_NAME = ".xefy-stamp.json"
# Libraries Xefy links. Only the ones that exist as ninja targets in the checkout
# are requested (checked after `gn gen`).
WANTED_NINJA_TARGETS = ("skia", "skparagraph", "skshaper", "skunicode",
                        "skunicode_core", "skunicode_icu")


def lib_name(target):
    return "skia.lib" if hostmod.target_os(target) == "windows" else "libskia.a"


def args_file(target):
    return os.path.join(ARGS_DIR, target + ".gn")


def out_dir(dep, target):
    return os.path.join(dep.path, "out", target)


def stamp_path(dep, target):
    return os.path.join(out_dir(dep, target), STAMP_NAME)


def parse_gn_args(text):
    """Parse GN args text into {key: value-string}; ignores comments, blanks and order."""
    result = {}
    for raw in text.splitlines():
        line = _strip_comment(raw).strip()
        if not line:
            continue
        if "=" not in line:
            raise ToolError("cannot parse GN args line: %r" % raw)
        key, value = line.split("=", 1)
        result[key.strip()] = " ".join(value.split())
    return result


def _strip_comment(line):
    in_str = False
    for i, ch in enumerate(line):
        if ch == '"':
            in_str = not in_str
        elif ch == "#" and not in_str:
            return line[:i]
    return line


def read_args(path):
    with open(path, "r") as f:
        return parse_gn_args(f.read())


def _read_stamp(dep, target):
    try:
        return read_json(stamp_path(dep, target))
    except (IOError, OSError, ValueError):
        return None


def status(dep, target):
    """Return (state, detail). States: built, adoptable, stale-revision, stale-args,
    not-built, unsupported."""
    src = args_file(target)
    if not os.path.isfile(src):
        return "unsupported", "no GN args for target %s (tools/skia_args/%s.gn)" % (target, target)
    out = out_dir(dep, target)
    lib = os.path.join(out, lib_name(target))
    if not os.path.isfile(lib):
        return "not-built", "%s missing" % rel(lib)
    stamp = _read_stamp(dep, target)
    if stamp is None:
        out_args = os.path.join(out, "args.gn")
        if os.path.isfile(out_args) and read_args(out_args) == read_args(src):
            return "adoptable", "existing build matches %s, no stamp" % rel(src)
        return "not-built", "output dir has no stamp and args differ"
    if stamp.get("skia_revision") != dep.revision:
        return "stale-revision", "built from %s" % str(stamp.get("skia_revision"))[:12]
    if stamp.get("args_sha256") != sha256_file(src):
        return "stale-args", "args file changed since last build"
    return "built", "stamp matches pinned revision and args"


def _write_stamp(dep, target, how):
    write_json(stamp_path(dep, target), {
        "target": target,
        "skia_revision": dep.revision,
        "args_sha256": sha256_file(args_file(target)),
        "how": how,
    })


def ninja_targets(out):
    rc, text, _ = run(["ninja", "-C", out, "-t", "targets", "all"],
                      check=False, capture=True, echo=False)
    names = set()
    if rc == 0:
        for line in text.splitlines():
            names.add(line.split(":", 1)[0].strip())
    return names


def build(dep, target):
    """Ensure Skia is built for target. Returns a short result string."""
    if target != hostmod.detect_host():
        raise ToolError("cannot build Skia for %s on a %s host; run setup on the target machine"
                        % (target, hostmod.detect_host()))
    st, detail = status(dep, target)
    out = out_dir(dep, target)
    if st == "unsupported":
        raise ToolError(detail)
    if st == "built":
        info("  skia     already built for %s (%s)" % (target, rel(out)))
        return "up to date"
    if st == "adoptable":
        _write_stamp(dep, target, "adopted")
        info("  skia     adopted existing build at %s (args match, wrote stamp)" % rel(out))
        return "adopted existing build"
    if st == "stale-args" and os.path.isfile(os.path.join(out, "args.gn")) and \
            read_args(os.path.join(out, "args.gn")) == read_args(args_file(target)):
        _write_stamp(dep, target, "restamped")
        info("  skia     args file changed only cosmetically; restamped %s" % rel(out))
        return "restamped"

    info("  skia     building for %s (%s): this takes a long time on a fresh checkout"
         % (target, detail))
    if not os.path.isfile(os.path.join(dep.path, "tools", "git-sync-deps")):
        raise ToolError("%s does not look like a Skia checkout (tools/git-sync-deps missing)"
                        % rel(dep.path))
    py = sys.executable or "python3"
    run([py, "tools/git-sync-deps"], cwd=dep.path)
    run([py, "bin/fetch-gn"], cwd=dep.path)
    gn = os.path.join(dep.path, "bin", "gn.exe" if hostmod.host_os() == "windows" else "gn")
    if not os.path.isfile(gn):
        raise ToolError("bin/fetch-gn did not produce %s" % rel(gn))
    os.makedirs(out, exist_ok=True)
    with open(args_file(target), "r") as f:
        args_text = f.read()
    with open(os.path.join(out, "args.gn"), "w") as f:
        f.write(args_text)
    run([gn, "gen", os.path.join("out", target)], cwd=dep.path)
    available = ninja_targets(out)
    targets = [t for t in WANTED_NINJA_TARGETS if t in available]
    if "skia" not in targets:
        raise ToolError("ninja target 'skia' not found in %s after gn gen" % rel(out))
    skipped = [t for t in WANTED_NINJA_TARGETS if t not in available]
    if skipped:
        info("  skia     note: ninja targets not present, skipped: %s" % ", ".join(skipped))
    run(["ninja", "-C", out] + targets)
    lib = os.path.join(out, lib_name(target))
    if not os.path.isfile(lib):
        raise ToolError("build finished but %s is missing" % rel(lib))
    _write_stamp(dep, target, "built")
    info("  skia     built %s" % rel(lib))
    return "built"


def clean_stamps(dep):
    """Paths of Xefy stamp files under skia/out/* (the build outputs are left alone)."""
    out_root = os.path.join(dep.path, "out")
    found = []
    if os.path.isdir(out_root):
        for name in sorted(os.listdir(out_root)):
            p = os.path.join(out_root, name, STAMP_NAME)
            if os.path.isfile(p):
                found.append(p)
    return found
