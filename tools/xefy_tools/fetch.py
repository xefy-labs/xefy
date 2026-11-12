"""Fetch pinned git checkouts. Never deletes or moves an existing checkout."""
import os

from .util import ToolError, info, out_of, run, rel

MISSING = "missing"
OK = "at pinned rev"
DIFFERENT = "at different rev"
INCOMPLETE = "incomplete"
NOT_GIT = "not a git checkout"


def _is_empty_dir(path):
    return os.path.isdir(path) and not os.listdir(path)


def head_of(path):
    return out_of(["git", "rev-parse", "--verify", "HEAD"], cwd=path)


def _toplevel_is_here(path):
    top = out_of(["git", "rev-parse", "--show-toplevel"], cwd=path)
    return top is not None and os.path.realpath(top) == os.path.realpath(path)


def state(dep):
    """Return (state, detail) for a dependency checkout."""
    path = dep.path
    if not os.path.exists(path) or _is_empty_dir(path):
        return MISSING, ""
    if not os.path.isdir(path) or not _toplevel_is_here(path):
        # Either not a git repo, or nested inside another repo (e.g. the Xefy repo).
        if os.path.isdir(os.path.join(path, ".git")):
            return INCOMPLETE, "git metadata present but unreadable"
        return NOT_GIT, ""
    head = head_of(path)
    if head is None:
        # git init done but nothing checked out: an interrupted earlier fetch.
        entries = [e for e in os.listdir(path) if e != ".git"]
        if not entries:
            return INCOMPLETE, "interrupted fetch (no commit checked out)"
        return NOT_GIT, ""
    if head == dep.revision:
        return OK, head
    return DIFFERENT, head


def fetch(dep):
    """Make sure dep is checked out at its pinned revision. Returns a status string.

    Raises ToolError (with guidance) if an existing checkout cannot be used.
    """
    st, detail = state(dep)
    dest = rel(dep.path)
    if st == OK:
        info("  %-8s already at pinned revision %s" % (dep.name, dep.short))
        return "present"
    if st == DIFFERENT:
        raise ToolError(
            "%s: checkout %s is at %s but deps.lock.json pins %s.\n"
            "  Xefy never modifies or deletes an existing checkout automatically.\n"
            "  To use the pinned revision, move the checkout aside (or delete it yourself)\n"
            "  and re-run `python3 xefy.py setup`; or, to keep your revision, update\n"
            "  deps.lock.json deliberately." % (dep.name, dest, detail[:12], dep.revision[:12]))
    if st == NOT_GIT:
        raise ToolError(
            "%s: %s exists and is not an empty directory or a git checkout of its own.\n"
            "  Move it aside and re-run `python3 xefy.py setup`." % (dep.name, dest))
    if st == INCOMPLETE and not os.path.isdir(os.path.join(dep.path, ".git")):
        raise ToolError("%s: %s looks corrupt; move it aside and re-run." % (dep.name, dest))

    info("  %-8s fetching %s (shallow, single commit) -> %s" % (dep.name, dep.short, dest))
    os.makedirs(dep.path, exist_ok=True)
    if not os.path.isdir(os.path.join(dep.path, ".git")):
        run(["git", "init", "-q"], cwd=dep.path)
    # Point origin at the pinned url (idempotent for an interrupted earlier attempt).
    if out_of(["git", "remote", "get-url", "origin"], cwd=dep.path) is None:
        run(["git", "remote", "add", "origin", dep.url], cwd=dep.path)
    elif out_of(["git", "remote", "get-url", "origin"], cwd=dep.path) != dep.url:
        run(["git", "remote", "set-url", "origin", dep.url], cwd=dep.path)
    run(["git", "fetch", "--depth", "1", "origin", dep.revision], cwd=dep.path)
    run(["git", "checkout", "-q", "FETCH_HEAD"], cwd=dep.path)
    head = head_of(dep.path)
    if head != dep.revision:
        raise ToolError("%s: after checkout HEAD is %s, expected %s"
                        % (dep.name, head, dep.revision))
    return "fetched"
