"""Small shared helpers: repo paths, subprocess running, hashing."""
import hashlib
import json
import os
import shutil
import subprocess
import sys

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT = os.path.dirname(TOOLS_DIR)


class ToolError(Exception):
    """A user-facing failure. The CLI prints the message and exits non-zero."""


def info(msg):
    print(msg)
    sys.stdout.flush()


def warn(msg):
    print("warning: " + msg, file=sys.stderr)
    sys.stderr.flush()


def which(name):
    return shutil.which(name)


def fmt_cmd(cmd):
    return " ".join(_quote(str(c)) for c in cmd)


def _quote(s):
    if s and all(ch.isalnum() or ch in "-_./=:+@%," for ch in s):
        return s
    return "'" + s.replace("'", "'\\''") + "'"


def run(cmd, cwd=None, env=None, check=True, capture=False, echo=True, input_text=None):
    """Run a subprocess.

    capture=True returns (returncode, stdout, stderr) text; otherwise output streams
    through to the terminal and the return value is the return code. With check=True a
    non-zero exit raises ToolError naming the command, cwd and exit code.
    """
    cmd = [str(c) for c in cmd]
    if echo:
        where = " (in %s)" % cwd if cwd else ""
        info("$ %s%s" % (fmt_cmd(cmd), where))
    try:
        if capture:
            proc = subprocess.run(
                cmd, cwd=cwd, env=env, input=input_text,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True,
            )
            out, err = proc.stdout, proc.stderr
        else:
            proc = subprocess.run(cmd, cwd=cwd, env=env, input=input_text,
                                  universal_newlines=True)
            out = err = ""
    except FileNotFoundError:
        if check:
            raise ToolError("command not found: %s" % cmd[0])
        return (127, "", "command not found: %s" % cmd[0]) if capture else 127
    except OSError as e:
        if check:
            raise ToolError("failed to run %s: %s" % (cmd[0], e))
        return (126, "", str(e)) if capture else 126
    if check and proc.returncode != 0:
        detail = ""
        if capture:
            tail = (err or out).strip().splitlines()[-15:]
            detail = "\n" + "\n".join("    " + l for l in tail)
        raise ToolError("command failed with exit code %d: %s%s%s" % (
            proc.returncode, fmt_cmd(cmd),
            " (in %s)" % cwd if cwd else "", detail))
    if capture:
        return proc.returncode, out, err
    return proc.returncode


def out_of(cmd, cwd=None, env=None):
    """Run quietly; return stripped stdout or None on any failure."""
    try:
        rc, out, _ = run(cmd, cwd=cwd, env=env, check=False, capture=True, echo=False)
    except Exception:
        return None
    return out.strip() if rc == 0 else None


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def read_json(path):
    with open(path, "r") as f:
        return json.load(f)


def write_json(path, data):
    tmp = path + ".tmp"
    with open(tmp, "w") as f:
        json.dump(data, f, indent=2, sort_keys=True)
        f.write("\n")
    os.replace(tmp, path)


def rel(path, root=ROOT):
    try:
        return os.path.relpath(path, root)
    except ValueError:
        return path


def run_parallel(jobs, cwd=None, workers=None):
    """Run [(label, cmd), ...] concurrently, capturing output.

    On any failure prints the captured compiler output of the failed jobs and raises
    ToolError. Returns nothing on success.
    """
    from concurrent.futures import ThreadPoolExecutor

    workers = workers or max(1, (os.cpu_count() or 2))
    failures = []

    def one(job):
        label, cmd = job
        rc, out, err = run(cmd, cwd=cwd, check=False, capture=True, echo=False)
        return label, cmd, rc, out, err

    done = 0
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for label, cmd, rc, out, err in pool.map(one, jobs):
            done += 1
            if rc != 0:
                failures.append((label, cmd, rc, out, err))
                info("  [%d/%d] FAILED %s" % (done, len(jobs), label))
            else:
                info("  [%d/%d] %s" % (done, len(jobs), label))
    if failures:
        for label, cmd, rc, out, err in failures[:3]:
            lines = (err or out).strip().splitlines()
            print("---- %s (exit %d) ----" % (label, rc), file=sys.stderr)
            print(fmt_cmd(cmd), file=sys.stderr)
            print("\n".join(lines[:40]), file=sys.stderr)
        raise ToolError("%d of %d compile job(s) failed (first: %s)"
                        % (len(failures), len(jobs), failures[0][0]))
