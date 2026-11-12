# Xefy

Xefy is a cross-platform UI framework written in Prismio. Apps describe their UI in a
Compose-style API; Xefy lays it out, paints it with Skia, and runs it on macOS, Windows, Linux
and XefyOS (a desktop OS on the Linux kernel). It reuses Flutter's source where that saves work,
and depends on neither Flutter nor Dart.

## Status

Xefy is an early scaffold. It has the directory structure, setup tooling, a draft C ABI header
(`engine/abi/xefy_engine.h`), a CLI skeleton (`cli/`) and an app template (`templates/app/`).
There is no rendering framework yet. The `engine/skia_c`, `engine/embedder`, `engine/three`,
`framework`, `os`, `examples` and `tests` directories contain no code yet.

## Requirements

- Python 3.9 or later
- git
- ninja
- clang, or the Xcode command line tools on macOS
- Prismio 0.2.0 or later

## Getting started

Check that the required tools are available:

```bash
python3 xefy.py doctor
```

Fetch the dependencies pinned in `deps.lock.json` and build Skia for the host. A Skia build from
scratch takes about an hour. Later runs are skipped by a stamp file.

```bash
python3 xefy.py setup
```

## Repository layout

| Directory | Purpose |
|---|---|
| `cli/` | The `xefy` tool that creates and builds apps, written in Prismio |
| `engine/abi/` | `xefy_engine.h`, the only native surface Prismio sees |
| `engine/skia_c/` | Drawing and text functions of the ABI, implemented on Skia |
| `engine/embedder/` | Window, input and platform code, one subdirectory per platform |
| `engine/three/` | Optional bgfx scene rendering |
| `framework/` | The Prismio packages (`xefy.kit`, `xefy.compose`, and others) |
| `os/` | XefyOS: compositor, shell and `.xapp` packaging |
| `templates/app/` | The app template that `xefy create` copies |
| `third_party/` | Fetched by setup and gitignored: Skia, Flutter (reference), bgfx, bx, bimg |
| `vendor/flutter/` | Copied Flutter C++, checked in with `PROVENANCE.json` |
| `examples/`, `tests/` | Sample apps and tests |

## Documentation

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): layers, rendering model, Flutter reuse, milestones
- [docs/config-xf.md](docs/config-xf.md): the format of an app's `config.xf`

## Licensing

Xefy is licensed under the Apache License 2.0; see `LICENSE`. Code derived from Flutter keeps
its BSD-3 licence, whose text is in `licenses/FLUTTER-LICENSE.txt`.
