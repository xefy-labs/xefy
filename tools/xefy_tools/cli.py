"""Command line interface for xefy.py."""
import argparse
import os
import shutil
import sys

from . import bgfx as bgfxmod
from . import doctor as doctormod
from . import fetch as fetchmod
from . import host as hostmod
from . import provenance as provmod
from . import skia as skiamod
from .lock import by_name, load_lock, with_feature
from .util import ROOT, ToolError, info, rel


def _parser():
    p = argparse.ArgumentParser(
        prog="xefy.py", description="Xefy developer tooling (stdlib-only, Python 3.9+).")
    sub = p.add_subparsers(dest="command", metavar="<command>")
    sub.required = True

    d = sub.add_parser("doctor", help="check tools, checkouts and builds")
    d.add_argument("--target", help="target to check (default: host)")

    s = sub.add_parser("setup", help="fetch pinned dependencies and build Skia")
    s.add_argument("--with", dest="features", action="append", default=[],
                   choices=["three"], metavar="FEATURE",
                   help="optional feature to also fetch and build: three (bgfx/bx/bimg)")
    s.add_argument("--target", help="target name (default: host); must match the host")
    s.add_argument("--skip-reference", action="store_true",
                   help="do not fetch reference-only checkouts (flutter, ~1 GB)")

    sub.add_parser("provenance", help="verify vendor/flutter attribution records")

    c = sub.add_parser("clean", help="remove Xefy's own build stamps/outputs (not checkouts)")
    c.add_argument("--dry-run", action="store_true", help="only list what would be removed")
    return p


def cmd_setup(args):
    host = hostmod.detect_host()
    target = hostmod.validate_target(args.target or host)
    three = "three" in args.features
    deps = load_lock()

    info("host: %s   target: %s%s" % (host, target, "   features: three" if three else ""))
    info("")
    trows = doctormod.check_tools(host, with_three=three)
    doctormod.print_table("Tool checks", trows)
    info("")
    if not doctormod.tools_ok(trows):
        raise ToolError("required tools are missing (see above); fix them and re-run setup")

    wanted = [d for d in deps if d.role == "required"]
    if not args.skip_reference:
        wanted += [d for d in deps if d.role == "reference"]
    if three:
        wanted += with_feature(deps, "three")

    info("Fetching pinned dependencies")
    failures = []
    for d in wanted:
        try:
            fetchmod.fetch(d)
        except ToolError as e:
            failures.append((d, e))
            print("error: %s" % e, file=sys.stderr)
    if failures:
        raise ToolError("%d dependency checkout(s) unusable: %s"
                        % (len(failures), ", ".join(d.name for d, _ in failures)))
    info("")

    info("Skia")
    skia_result = skiamod.build(by_name(deps, "skia"), target)
    info("")

    if three:
        info("bgfx (feature three)")
        bg = dict((d.name, d) for d in with_feature(deps, "three"))
        bgfxmod.build(bg, target)
        info("")
    info("setup complete (skia: %s)" % skia_result)
    return 0


def cmd_clean(args):
    """Remove only Xefy's own stamps and bgfx build output, never checkouts or Skia's
    (hour-long) build outputs."""
    deps = load_lock()
    third_party = os.path.realpath(os.path.join(ROOT, "third_party"))
    victims = []
    for p in skiamod.clean_stamps(by_name(deps, "skia")):
        victims.append((p, "file"))
    for d in with_feature(deps, "three"):
        if d.name == "bgfx":
            p = os.path.join(d.path, ".xefy-build")
            if os.path.isdir(p):
                victims.append((p, "dir"))
    if not victims:
        info("nothing to clean")
        return 0
    for p, kind in victims:
        real = os.path.realpath(p)
        if not real.startswith(third_party + os.sep):
            raise ToolError("refusing to remove %s: outside third_party/" % p)
        info("%s %s" % ("would remove" if args.dry_run else "removing", rel(p)))
        if args.dry_run:
            continue
        if kind == "dir":
            shutil.rmtree(p)
        else:
            os.remove(p)
    info("Skia build outputs (third_party/skia/out/*) were kept; only stamps were removed.")
    return 0


def main(argv=None):
    args = _parser().parse_args(argv)
    try:
        if args.command == "doctor":
            target = args.target and hostmod.validate_target(args.target)
            return doctormod.run_doctor(target)
        if args.command == "setup":
            return cmd_setup(args)
        if args.command == "provenance":
            return provmod.run_provenance()
        if args.command == "clean":
            return cmd_clean(args)
    except ToolError as e:
        print("error: %s" % e, file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("interrupted", file=sys.stderr)
        return 130
    return 2
