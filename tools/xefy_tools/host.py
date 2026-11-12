"""Host and target detection. Target names look like macos-arm64, linux-x64."""
import platform
import sys

SUPPORTED_TARGETS = ("macos-arm64", "linux-x64", "linux-arm64", "windows-x64")

_OS = {"darwin": "macos", "linux": "linux", "win32": "windows", "cygwin": "windows"}
_ARCH = {
    "arm64": "arm64", "aarch64": "arm64", "armv8l": "arm64",
    "x86_64": "x64", "amd64": "x64", "x64": "x64",
}


def host_os():
    for prefix, name in _OS.items():
        if sys.platform.startswith(prefix):
            return name
    return sys.platform


def host_arch():
    m = platform.machine().lower()
    return _ARCH.get(m, m)


def detect_host():
    return "%s-%s" % (host_os(), host_arch())


def target_os(target):
    return target.split("-", 1)[0]


def target_arch(target):
    return target.split("-", 1)[1] if "-" in target else ""


def validate_target(target):
    if target not in SUPPORTED_TARGETS:
        from .util import ToolError
        raise ToolError("unknown target %r; known targets: %s"
                        % (target, ", ".join(SUPPORTED_TARGETS)))
    return target
