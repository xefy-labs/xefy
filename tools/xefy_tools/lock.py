"""Load and validate deps.lock.json."""
import json
import os
import re

from .util import ROOT, ToolError

LOCK_PATH = os.path.join(ROOT, "deps.lock.json")
ROLES = ("required", "reference", "optional")
_SHA_RE = re.compile(r"^[0-9a-f]{40}$")
_NAME_RE = re.compile(r"^[A-Za-z0-9_.-]+$")


class Dep(object):
    def __init__(self, d):
        self.name = d["name"]
        self.url = d["url"]
        self.revision = d["revision"]
        self.dest = d["dest"]
        self.role = d["role"]
        self.feature = d.get("feature")
        self.builder = d.get("builder")
        self.note = d.get("note", "")

    @property
    def path(self):
        return os.path.join(ROOT, *self.dest.split("/"))

    @property
    def short(self):
        return self.revision[:10]

    def __repr__(self):
        return "Dep(%s@%s)" % (self.name, self.short)


def _fail(msg):
    raise ToolError("deps.lock.json: " + msg)


def load_lock(path=LOCK_PATH):
    try:
        with open(path, "r") as f:
            data = json.load(f)
    except FileNotFoundError:
        raise ToolError("missing %s" % path)
    except ValueError as e:
        raise ToolError("deps.lock.json is not valid JSON: %s" % e)
    if not isinstance(data, dict) or data.get("schema") != 1:
        _fail('expected an object with "schema": 1')
    entries = data.get("dependencies")
    if not isinstance(entries, list) or not entries:
        _fail('"dependencies" must be a non-empty list')
    deps, names, dests = [], set(), set()
    for i, d in enumerate(entries):
        where = "dependencies[%d]" % i
        if not isinstance(d, dict):
            _fail("%s must be an object" % where)
        for key in ("name", "url", "revision", "dest", "role"):
            if not isinstance(d.get(key), str) or not d[key]:
                _fail("%s: missing or non-string %r" % (where, key))
        where = "%s (%s)" % (where, d["name"])
        if not _NAME_RE.match(d["name"]):
            _fail("%s: bad name" % where)
        if d["name"] in names:
            _fail("%s: duplicate name" % where)
        names.add(d["name"])
        if not _SHA_RE.match(d["revision"]):
            _fail("%s: revision must be a full 40-character lowercase sha" % where)
        dest = d["dest"]
        parts = dest.split("/")
        if (os.path.isabs(dest) or "\\" in dest or len(parts) < 2
                or parts[0] != "third_party" or ".." in parts or "" in parts):
            _fail("%s: dest must be a relative path under third_party/" % where)
        if dest in dests:
            _fail("%s: duplicate dest" % where)
        dests.add(dest)
        if d["role"] not in ROLES:
            _fail("%s: role must be one of %s" % (where, ", ".join(ROLES)))
        if d["role"] == "optional" and not d.get("feature"):
            _fail("%s: optional dependencies need a \"feature\"" % where)
        if not (d["url"].startswith("https://") or d["url"].startswith("git@")):
            _fail("%s: url must be https or ssh" % where)
        deps.append(Dep(d))
    return deps


def by_name(deps, name):
    for d in deps:
        if d.name == name:
            return d
    raise ToolError("deps.lock.json has no dependency named %r" % name)


def with_feature(deps, feature):
    return [d for d in deps if d.feature == feature]
