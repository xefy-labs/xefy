"""`xefy.py doctor`: check tools, dependency checkouts and the Skia build."""
import os
import re
import sys

from . import bgfx as bgfxmod
from . import fetch as fetchmod
from . import host as hostmod
from . import skia as skiamod
from .lock import by_name, load_lock
from .util import ToolError, info, out_of, rel, which

OK, FAIL, WARN, INFO = "ok", "MISSING", "warn", "info"
MIN_PYTHON = (3, 9)
MIN_PRISMIO = (0, 2, 0)


class Row(object):
    def __init__(self, name, status, detail, required=True):
        self.name, self.status, self.detail, self.required = name, status, detail, required

    @property
    def failed(self):
        return self.status == FAIL and self.required


def _ver(text):
    m = re.search(r"(\d+)\.(\d+)(?:\.(\d+))?", text or "")
    return tuple(int(x) if x else 0 for x in m.groups()) if m else None


def _fmt(v):
    return ".".join(str(x) for x in v)


def check_tools(host_target, with_three=False):
    rows = []
    v = sys.version_info
    ok = (v.major, v.minor) >= MIN_PYTHON
    rows.append(Row("python >= %s" % _fmt(MIN_PYTHON), OK if ok else FAIL,
                    "%d.%d.%d" % (v.major, v.minor, v.micro)))

    git = which("git")
    rows.append(Row("git", OK if git else FAIL,
                    (out_of(["git", "--version"]) or git) if git else "not found on PATH"))

    ninja = which("ninja")
    rows.append(Row("ninja", OK if ninja else FAIL,
                    ("ninja " + (out_of(["ninja", "--version"]) or "?")) if ninja
                    else "not found on PATH (macOS: brew install ninja)"))

    clang = which("clang")
    if clang:
        first = (out_of(["clang", "--version"]) or "").splitlines()
        rows.append(Row("C/C++ compiler (clang)", OK, (first[0] if first else clang)))
    else:
        rows.append(Row("C/C++ compiler (clang)", FAIL, "clang not found on PATH"))

    if hostmod.host_os() == "macos":
        clt = out_of(["xcode-select", "-p"])
        rows.append(Row("Xcode command line tools", OK if clt else FAIL,
                        clt or "not installed (run: xcode-select --install)"))

    pr = which("prismio")
    if not pr:
        rows.append(Row("prismio >= %s" % _fmt(MIN_PRISMIO), FAIL, "not found on PATH"))
    else:
        out = out_of(["prismio", "--version"])
        first = out.splitlines()[0] if out else ""
        ver = _ver(first)
        if ver is None:
            rows.append(Row("prismio >= %s" % _fmt(MIN_PRISMIO), FAIL,
                            "could not parse `prismio --version` output: %r" % first))
        else:
            rows.append(Row("prismio >= %s" % _fmt(MIN_PRISMIO),
                            OK if ver >= MIN_PRISMIO else FAIL, first.strip()))

    ar = which("ar")
    rows.append(Row("ar (for --with three)", OK if ar else FAIL if with_three else INFO,
                    ar or "not found on PATH", required=with_three))
    return rows


def dep_rows(deps):
    rows = []
    for d in deps:
        st, detail = fetchmod.state(d)
        required = d.role == "required"
        label = "%s (%s%s)" % (d.name, d.role, ", " + d.feature if d.feature else "")
        if st == fetchmod.OK:
            rows.append(Row(label, OK, "at pinned rev %s" % d.short, required))
        elif st == fetchmod.MISSING:
            status = FAIL if required else INFO
            rows.append(Row(label, status, "missing (pinned %s)" % d.short, required))
        elif st == fetchmod.DIFFERENT:
            status = FAIL if required else WARN
            rows.append(Row(label, status, "at different rev %s (pinned %s)"
                            % (detail[:10], d.short), required))
        else:
            status = FAIL if required else WARN
            rows.append(Row(label, status, "%s %s" % (st, detail), required))
    return rows


def skia_row(deps, target):
    skia = by_name(deps, "skia")
    label = "Skia library (%s)" % target
    if fetchmod.state(skia)[0] == fetchmod.MISSING:
        return Row(label, FAIL, "skia checkout missing; run setup")
    st, detail = skiamod.status(skia, target)
    if st == "built":
        return Row(label, OK, "built: %s" % rel(skiamod.out_dir(skia, target)))
    if st == "adoptable":
        return Row(label, WARN, "existing build found, not stamped yet (setup adopts it)",
                   required=False)
    if st == "unsupported":
        return Row(label, FAIL, detail)
    return Row(label, FAIL, "not built (%s); run: python3 xefy.py setup" % detail)


def bgfx_row(deps, target):
    from .lock import with_feature
    three = with_feature(deps, "three")
    bg = [d for d in three if d.name == "bgfx"]
    label = "bgfx libraries (%s)" % target
    if not bg:
        return Row(label, INFO, "not configured", required=False)
    if bgfxmod.is_built(bg[0], target):
        return Row(label, OK, "built: %s" % rel(bgfxmod.build_dir(bg[0], target)),
                   required=False)
    return Row(label, INFO, "not built (optional, feature three: setup --with three)",
               required=False)


def print_table(title, rows):
    print(title)
    wn = max([len(r.name) for r in rows] + [4])
    for r in rows:
        mark = {OK: "[ ok ]", FAIL: "[FAIL]", WARN: "[warn]", INFO: "[info]"}[r.status]
        if r.status == FAIL and not r.required:
            mark = "[warn]"
        print("  %s %-*s  %s" % (mark, wn, r.name, r.detail))
    sys.stdout.flush()


def tools_ok(rows):
    return not any(r.failed for r in rows)


def run_doctor(target=None):
    host = hostmod.detect_host()
    target = target or host
    deps = load_lock()
    print("host: %s   target: %s" % (host, target))
    print()
    trows = check_tools(host)
    print_table("Tools", trows)
    print()
    drows = dep_rows(deps)
    print_table("Dependencies (deps.lock.json)", drows)
    print()
    brows = [skia_row(deps, target), bgfx_row(deps, target)]
    print_table("Builds", brows)
    print()
    failed = [r for r in trows + drows + brows if r.failed]
    if failed:
        print("doctor: %d required item(s) missing: %s" % (len(failed),
              ", ".join(r.name for r in failed)))
        return 1
    print("doctor: all required items present")
    return 0
