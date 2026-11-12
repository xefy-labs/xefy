# 0005. Skia for 2D, bgfx for 3D only

Status: accepted (2026-11-12)

## Context
Skia is a 2D library with its own GPU backends. bgfx is a rendering abstraction with its own
device. Both cannot own one window surface.

## Decision
- Skia draws all UI and owns the window surface and presentation: Metal on macOS, Vulkan or GL
  on Linux, Direct3D or Vulkan on Windows.
- bgfx renders 3D scenes into an offscreen texture. The UI draws that texture through a
  `Scene3D` composable like an image.
- bgfx is optional: the `three` feature in `config.xf`, the `xefy.three` package, and
  `python3 xefy.py setup --with three`. An app without it links no bgfx.

## Consequences
Nothing in the core framework may depend on bgfx. Zero-copy texture sharing between the two
needs a prototype per graphics API before `xefy.three` is designed; a copy through CPU memory is
the fallback and is acceptable for a first version.
