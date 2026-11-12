"""Guard for code copied from Flutter (BSD-3-Clause): every file under vendor/flutter/
must be listed in vendor/flutter/PROVENANCE.json so attribution is never lost.

PROVENANCE.json: {"files": [{"path", "upstream_path", "revision", "changes"}, ...]}
  path           file path relative to vendor/flutter/, forward slashes
  upstream_path  path of the original file in the flutter repo
  revision       40-char flutter commit the file was copied from
  changes        what was modified relative to upstream (use "none" if verbatim)
README.md and PROVENANCE.json at the top of vendor/flutter/ are exempt (so is the
macOS .DS_Store file).
"""
import json
import os
import re

from .util import ROOT, info

VENDOR_DIR = os.path.join(ROOT, "vendor", "flutter")
PROVENANCE_PATH = os.path.join(VENDOR_DIR, "PROVENANCE.json")
EXEMPT_TOP = ("README.md", "PROVENANCE.json")
IGNORED_NAMES = (".DS_Store",)
FIELDS = ("path", "upstream_path", "revision", "changes")
_SHA = re.compile(r"^[0-9a-f]{40}$")


def _actual_files():
    found = set()
    for dirpath, dirnames, filenames in os.walk(VENDOR_DIR):
        dirnames.sort()
        for name in filenames:
            if name in IGNORED_NAMES:
                continue
            full = os.path.join(dirpath, name)
            relpath = os.path.relpath(full, VENDOR_DIR).replace(os.sep, "/")
            if relpath in EXEMPT_TOP:
                continue
            found.add(relpath)
    return found


def check():
    """Return a list of violation strings (empty means OK)."""
    problems = []
    if not os.path.isdir(VENDOR_DIR):
        return ["vendor/flutter/ does not exist"]
    try:
        with open(PROVENANCE_PATH, "r") as f:
            data = json.load(f)
    except FileNotFoundError:
        return ["vendor/flutter/PROVENANCE.json is missing"]
    except ValueError as e:
        return ["vendor/flutter/PROVENANCE.json is not valid JSON: %s" % e]
    entries = data.get("files") if isinstance(data, dict) else None
    if not isinstance(entries, list):
        return ['vendor/flutter/PROVENANCE.json must be an object with a "files" list']

    listed = {}
    for i, e in enumerate(entries):
        tag = "entry %d" % i
        if not isinstance(e, dict):
            problems.append("%s: not an object" % tag)
            continue
        p = e.get("path")
        if isinstance(p, str) and p:
            tag = "entry %d (%s)" % (i, p)
        for field in FIELDS:
            v = e.get(field)
            if not isinstance(v, str) or not v.strip():
                problems.append("%s: missing or empty field %r" % (tag, field))
        rev = e.get("revision")
        if isinstance(rev, str) and rev.strip() and not _SHA.match(rev):
            problems.append("%s: revision must be a full 40-character lowercase sha" % tag)
        if isinstance(p, str) and p:
            norm = p.replace("\\", "/")
            if norm != p or p.startswith("/") or ".." in p.split("/") or p in EXEMPT_TOP:
                problems.append("%s: path must be relative to vendor/flutter/ with forward "
                                "slashes and no '..'" % tag)
                continue
            if p in listed:
                problems.append("%s: duplicate entry for path" % tag)
            listed[p] = e

    actual = _actual_files()
    for p in sorted(actual - set(listed)):
        problems.append("no PROVENANCE.json entry for vendor/flutter/%s" % p)
    for p in sorted(set(listed) - actual):
        problems.append("PROVENANCE.json entry points at missing file vendor/flutter/%s" % p)
    return problems


def run_provenance():
    problems = check()
    if problems:
        print("provenance check FAILED (%d problem(s)):" % len(problems))
        for p in problems:
            print("  - " + p)
        print("Every file copied from Flutter must keep attribution: add an entry with "
              "path, upstream_path, revision, changes to vendor/flutter/PROVENANCE.json.")
        return 1
    n = len(_actual_files())
    info("provenance OK: %d file(s) under vendor/flutter/, all recorded" % n)
    return 0
