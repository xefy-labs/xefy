"""Build bx, bimg and bgfx as static libraries (feature "three").

Instead of generating projects with GENie, the compiler is invoked directly. The
source lists, include dirs and defines below mirror, and must be kept in sync with:
  bx/scripts/bx.lua, bx/scripts/toolchain.lua       -> libbx.a
  bimg/scripts/bimg.lua                             -> libbimg.a
  bimg/scripts/bimg_decode.lua                      -> libbimg_decode.a
  bgfx/scripts/bgfx.lua (+ genie.lua)               -> libbgfx.a
Output: third_party/bgfx/.xefy-build/<target>/{libbx,libbimg,libbimg_decode,libbgfx}.a

Link line for consumers (macOS): libbgfx.a libbimg_decode.a libbimg.a libbx.a plus
-framework Cocoa QuartzCore IOKit OpenGL and -weak_framework Metal MetalKit VideoToolbox
CoreMedia CoreVideo (from bgfx.lua), and -lc++.

Note on bimg: its image decoder (imageParse) lives in a separate GENie project,
bimg_decode, which bgfx needs at link time, so it is built as libbimg_decode.a.
Everything is built as BX_CONFIG_DEBUG=0 / NDEBUG (a release configuration).
"""
import glob
import os

from . import host as hostmod
from .util import (ToolError, info, out_of, read_json, rel, run, run_parallel, which,
                   write_json)

RECIPE_VERSION = 2   # bump whenever sources/flags below change
STAMP_NAME = ".xefy-stamp.json"
LIBS = ("libbx.a", "libbimg.a", "libbimg_decode.a", "libbgfx.a")
IMPLEMENTED = ("macos-arm64", "linux-x64", "linux-arm64")
UNTESTED = ("linux-x64", "linux-arm64")


def build_dir(bgfx_dep, target):
    return os.path.join(bgfx_dep.path, ".xefy-build", target)


def is_built(bgfx_dep, target):
    d = build_dir(bgfx_dep, target)
    return all(os.path.isfile(os.path.join(d, l)) for l in LIBS)


class Toolchain(object):
    def __init__(self, target):
        self.target = target
        tos = hostmod.target_os(target)
        self.os = tos
        if tos == "macos":
            cc = out_of(["xcrun", "--find", "clang"]) or which("clang")
            cxx = out_of(["xcrun", "--find", "clang++"]) or which("clang++")
            self.ar = out_of(["xcrun", "--find", "ar"]) or which("ar")
            sdk = out_of(["xcrun", "--show-sdk-path"])
            arch = "arm64" if hostmod.target_arch(target) == "arm64" else "x86_64"
            self.common = ["-arch", arch, "-mmacosx-version-min=11.0"]
            if sdk:
                self.common += ["-isysroot", sdk]
        else:
            cc, cxx = which("clang"), which("clang++")
            self.ar = which("ar")
            self.common = ["-fPIC", "-pthread"]
        if not (cc and cxx and self.ar):
            raise ToolError("bgfx build needs clang, clang++ and ar on PATH")
        self.cc, self.cxx = cc, cxx
        self.version = (out_of([cxx, "--version"]) or "unknown").splitlines()[0]

    def compat_include(self, bx_dir):
        sub = {"macos": "osx", "linux": "linux", "windows": "msvc"}[self.os]
        return os.path.join(bx_dir, "include", "compat", sub)


def _defines():
    return ["-DNDEBUG", "-DBX_CONFIG_DEBUG=0",
            "-D__STDC_LIMIT_MACROS", "-D__STDC_FORMAT_MACROS", "-D__STDC_CONSTANT_MACROS"]


def _cxx_flags(tc):
    flags = ["-std=c++20", "-O2", "-fno-rtti", "-fno-exceptions"] + _defines() + tc.common
    if tc.os == "macos":
        flags.append("-funwind-tables")   # from bx.lua: needed for unwinding on Apple arm64
    return flags


def _c_flags(tc):
    return ["-std=c11", "-O2"] + _defines() + tc.common


def _inc(*dirs):
    out = []
    for d in dirs:
        out += ["-I", d]
    return out


def _glob(*parts):
    return sorted(glob.glob(os.path.join(*parts)))


def _obj_path(objdir, lib, src):
    safe = os.path.basename(src)
    return os.path.join(objdir, lib, safe + ".o")


def _compile_jobs(lib, srcs, base_cmd, objdir, extra_for=None):
    os.makedirs(os.path.join(objdir, lib), exist_ok=True)
    jobs, objs = [], []
    seen = set()
    for s in srcs:
        o = _obj_path(objdir, lib, s)
        if o in seen:
            raise ToolError("duplicate object name %s in %s" % (o, lib))
        seen.add(o)
        cmd = list(base_cmd)
        if extra_for:
            cmd += extra_for(s)
        cmd += ["-c", s, "-o", o]
        jobs.append(("%s: %s" % (lib, os.path.basename(s)), cmd))
        objs.append(o)
    return jobs, objs


def _archive(tc, out_lib, objs):
    if os.path.exists(out_lib):
        os.remove(out_lib)          # our own output; ar would otherwise append
    run([tc.ar, "rcs", out_lib] + objs, echo=False)
    info("  archived %s (%d objects)" % (os.path.basename(out_lib), len(objs)))


def _check_sources(deps_by_name):
    for name, d in deps_by_name.items():
        if not os.path.isdir(d.path):
            raise ToolError("%s checkout missing at %s; run `python3 xefy.py setup --with three`"
                            % (name, rel(d.path)))


def build(deps_by_name, target, force=False):
    """deps_by_name: {'bgfx': Dep, 'bx': Dep, 'bimg': Dep}."""
    if target not in IMPLEMENTED:
        raise ToolError("bgfx build not implemented for this target yet: %s "
                        "(implemented: %s)" % (target, ", ".join(IMPLEMENTED)))
    if target != hostmod.detect_host():
        raise ToolError("cannot build bgfx for %s on a %s host" % (target, hostmod.detect_host()))
    _check_sources(deps_by_name)
    bgfx_d, bx_d, bimg_d = (deps_by_name[n] for n in ("bgfx", "bx", "bimg"))
    tc = Toolchain(target)
    outdir = build_dir(bgfx_d, target)
    stamp_file = os.path.join(outdir, STAMP_NAME)
    want = {
        "recipe": RECIPE_VERSION,
        "target": target,
        "revisions": {n: d.revision for n, d in sorted(deps_by_name.items())},
        "compiler": tc.version,
    }
    if not force and is_built(bgfx_d, target):
        try:
            if read_json(stamp_file) == want:
                info("  bgfx     already built for %s (%s)" % (target, rel(outdir)))
                return "up to date"
        except (IOError, OSError, ValueError):
            pass
    if target in UNTESTED:
        info("  bgfx     note: %s build path is untested" % target)

    objdir = os.path.join(outdir, "obj")
    os.makedirs(outdir, exist_ok=True)
    if os.path.exists(stamp_file):
        os.remove(stamp_file)       # invalidate until the whole build succeeds
    info("  bgfx     building bx, bimg, bimg_decode, bgfx for %s with %s" % (target, tc.version))

    bx_inc = os.path.join(bx_d.path, "include")
    compat = tc.compat_include(bx_d.path)
    cxx = [tc.cxx] + _cxx_flags(tc)
    cc = [tc.cc] + _c_flags(tc)

    # ---- bx: one amalgamated translation unit -------------------------------------
    srcs = [os.path.join(bx_d.path, "src", "amalgamated.cpp")]
    jobs, objs = _compile_jobs(
        "bx", srcs, cxx + _inc(bx_inc, os.path.join(bx_d.path, "3rdparty"), compat), objdir)
    run_parallel(jobs)
    _archive(tc, os.path.join(outdir, "libbx.a"), objs)

    # ---- bimg: image.cpp + astc-encoder ------------------------------------------
    astc = os.path.join(bimg_d.path, "3rdparty", "astc-encoder")
    srcs = [os.path.join(bimg_d.path, "src", "image.cpp"),
            os.path.join(bimg_d.path, "src", "image_encode_wic.cpp")]   # stub off Windows
    srcs += _glob(astc, "source", "*.cpp")
    base = cxx + ["-DASTCENC_F16C=0", "-DASTCENC_NEON=0"] + _inc(
        bx_inc, compat, os.path.join(bimg_d.path, "include"), os.path.join(astc, "include"))
    jobs, objs = _compile_jobs("bimg", srcs, base, objdir)
    run_parallel(jobs)
    _archive(tc, os.path.join(outdir, "libbimg.a"), objs)

    # ---- bimg_decode: image_decode*.cpp + dav1d + libavif (C amalgamations) ------
    p3 = os.path.join(bimg_d.path, "3rdparty")
    dec_inc = _inc(bx_inc, compat, os.path.join(bimg_d.path, "include"), p3,
                   os.path.join(p3, "tinyexr", "deps"),
                   os.path.join(p3, "libavif"), os.path.join(p3, "libavif", "include"),
                   os.path.join(p3, "libavif", "third_party", "libyuv", "include"),
                   os.path.join(p3, "dav1d"), os.path.join(p3, "dav1d", "include"))
    cpp_srcs = _glob(bimg_d.path, "src", "image_decode*.cpp")
    c_srcs = [os.path.join(p3, "dav1d", n) for n in
              ("dav1d-amalgamated.c", "dav1d-bitdepth-8.c", "dav1d-bitdepth-16.c")]
    c_srcs.append(os.path.join(p3, "libavif", "libavif-amalgamated.c"))
    jobs, objs = _compile_jobs("bimg_decode", cpp_srcs, cxx + ["-DAVIF_CODEC_DAV1D"] + dec_inc,
                               objdir)
    cjobs, cobjs = _compile_jobs("bimg_decode", c_srcs, cc + ["-DAVIF_CODEC_DAV1D"] + dec_inc,
                                 objdir)
    run_parallel(jobs + cjobs)
    _archive(tc, os.path.join(outdir, "libbimg_decode.a"), objs + cobjs)

    # ---- bgfx: amalgamated.cpp (compiled as Objective-C++ on macOS, per bgfx.lua) --
    bg = bgfx_d.path
    inc = _inc(os.path.join(bg, "3rdparty"), os.path.join(bimg_d.path, "include"), bx_inc,
               compat, os.path.join(bg, "include"), os.path.join(bg, "3rdparty", "khronos"))
    if tc.os == "linux":
        dx = os.path.join(bg, "3rdparty", "directx-headers", "include")
        inc += _inc(os.path.join(dx, "directx"), dx, os.path.join(dx, "wsl", "stubs"))
    lang = ["-x", "objective-c++"] if tc.os == "macos" else []
    jobs, objs = _compile_jobs("bgfx", [os.path.join(bg, "src", "amalgamated.cpp")],
                               cxx + lang + inc, objdir)
    run_parallel(jobs)
    _archive(tc, os.path.join(outdir, "libbgfx.a"), objs)

    write_json(stamp_file, want)
    info("  bgfx     built libraries in %s" % rel(outdir))
    return "built"
