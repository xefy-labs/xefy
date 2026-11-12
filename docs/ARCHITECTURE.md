# Xefy architecture

Xefy is a cross-platform UI framework written in Prismio. Apps describe UI with **Xefy UI**,
a declarative API of composables in the style of Jetpack Compose, styled with **Tint**
classes. Xefy lays it out, paints it with Skia, and runs it on macOS, Windows, Linux and
AOS (a desktop OS on the Linux kernel). It reuses Flutter's source where that saves
work and depends on neither Flutter nor Dart.

## Where things are written down

| Document | Holds |
|---|---|
| `docs/adr/` | One record per decision, with its reason and consequences |
| `docs/FRAMEWORK_V0.md` | The framework as it is built now: modules, types, the frame loop |
| `docs/BUILD_PLAN.md` | Work packages: who owns which paths, what each depends on, how each is checked |
| `docs/COMPOSABLES.md` | The composable catalogue |
| `docs/THEME.md` | Porcelain: every colour, size, radius, shadow and duration |
| `docs/TINT.md` | The utility class vocabulary |
| `docs/APP_APIS.md` | App state, navigation, assets, strings, shortcuts, overlays |
| `docs/IDE_PLUGIN.md` | The JetBrains plugin and the JSON contract the `xefy` tool gives it |
| `docs/config-xf.md` | The app configuration format |
| `engine/abi/xefy_engine.h` | The native boundary |

## Layers

```
App code (Prismio, written with Xefy UI)
────────────────────────────────────────────────────────────────
xefy.ui         the built-in composables
xefy.tint       utility classes over the theme's tokens
xefy.runtime    Scope, remember, state, recomposition, modifiers
xefy.render     render tree: constraints, layout, paint, hit-test
xefy.gestures   xefy.animation   xefy.scheduler   xefy.semantics   xefy.text
xefy.gfx        Canvas, Paint, Path, Paragraph, Image (Prismio API over the C ABI)
xefy.three      optional: 3D scenes through bgfx
════════════════ engine/abi/xefy_engine.h (C ABI) ═══════════════
skia_c          C++ shim that implements the drawing and text half of the ABI on Skia
embedder        window, vsync, input, IME, clipboard, accessibility; one per platform
three           optional: bgfx renders a scene into a texture the canvas can draw
────────────────────────────────────────────────────────────────
macOS · Windows · Linux (Wayland, X11) · Linux DRM/KMS (AOS)
```

Rules that hold everywhere:

- **One native boundary.** Prismio sees `xefy_engine.h` and nothing else. C++ exists below it
  only where a C++ API (Skia) or a platform API (AppKit, Win32) forces it.
- **Prismio above the boundary.** Layout, painting logic, gestures, animation, text editing
  and the Compose runtime are Prismio. Do not add C or C++ for something Prismio can express.
- **Scripts are Python.** Setup and build orchestration live in `tools/`; there are no shell
  scripts.

## Compose in front, a render tree behind

Compose's runtime (composer, slot table, snapshot state) replaces Flutter's Widget and Element
layers. Under it sits a retained render tree whose protocols are ported from Flutter's
`rendering/` library: box constraints go down, sizes come up, the parent sets positions. That
library is the proven part of Flutter and the part worth porting rather than redesigning.

One frame:

1. The app's loop polls the engine for input and a vsync tick. Prismio owns the loop; the
   engine never calls back into it (decision 0004).
2. The root composable function runs and builds this frame's node tree.
3. Layout sizes and positions the nodes, then paint draws them through Skia into the window
   surface, which is presented.

That is the v0 runtime, which is what Prismio 0.2.0 can express: an explicit `scope` context,
immediate-mode events and a tree rebuilt each frame (decision 0008, `FRAMEWORK_V0.md`). The
target runtime re-runs only the composables whose state changed and keeps a retained tree. It
replaces v0 behind the same composable names once Prismio has the features listed in
`XEFY_PLAN.md`. Everything runs on one thread until Prismio has synchronisation types.

## Graphics

Skia does all 2D drawing and owns the window surface. Text shaping and layout use Skia's
`skparagraph` (HarfBuzz and ICU underneath).

bgfx is for 3D only. It renders a scene into an offscreen texture, and the UI draws that
texture through a `Scene3D` composable the way it draws an image. It is an optional feature
(`features { three = true }` in an app's `config.xf`), so apps without 3D never link it.
Sharing the texture between bgfx and Skia without a copy needs a prototype per graphics API
before `xefy.three` is built on it.

## Reusing Flutter

| Flutter source | What Xefy does |
|---|---|
| `packages/flutter/lib/src/{rendering,gestures,animation,scheduler,physics,painting,semantics}` and `services` text input (Dart) | Port to Prismio |
| `widgets/`, `material/`, `cupertino/` (Dart) | Not ported; `xefy.runtime` and `xefy.ui` replace them |
| Engine `flow/` (layer tree, raster cache) and `display_list/` (C++) | Copy into `vendor/flutter/`, or port |
| Engine `shell/platform/*` keyboard, IME and accessibility code (C++, Objective-C) | Copy selectively into the embedders |
| `lib/ui`, `runtime/`, `tonic`, Impeller, Dart VM glue | Not used |
| `txt/` | Not used; Skia's `skparagraph` directly |

`third_party/flutter` is a pinned checkout used only as the source to port from. It is never
built or linked.

Flutter is BSD-3 licensed. Every ported file names its upstream path and revision in a header
comment. Every copied file under `vendor/flutter/` has an entry in
`vendor/flutter/PROVENANCE.json`, checked by `python3 xefy.py provenance`. The licence text is
in `licenses/FLUTTER-LICENSE.txt`.

## Repository layout

```
Xefy/xefy/ is the SDK and the git repository; Xefy/website/ sits beside it.

xefy.py            entry point for repo tooling: doctor, setup, provenance, clean
deps.lock.json     pinned revisions of every third-party dependency
tools/             Python tooling behind xefy.py; GN args for Skia per target
third_party/       fetched by setup, gitignored: skia, flutter (reference), bgfx, bx, bimg
vendor/flutter/    copied Flutter C++ with PROVENANCE.json, checked in
engine/
  abi/             xefy_engine.h, the only native surface Prismio sees
  skia_c/          the ABI's drawing and text functions, implemented on Skia
  embedder/        macos, windows, linux_wayland, linux_x11, linux_drm
  three/           bgfx scene rendering (optional feature)
framework/         the Prismio packages listed under Layers
cli/               the `xefy` tool app developers use, written in Prismio
templates/app/     what `xefy create` copies
os/                AOS: compositor, shell, .xapp packaging
examples/  tests/  docs/  licenses/
```

Two tools with similar names do different jobs: `python3 xefy.py` prepares and builds *this
repository*; the `xefy` binary from `cli/` creates and builds *apps*.

## Setting up a clone

```bash
python3 xefy.py doctor
```

```bash
python3 xefy.py setup
```

`setup` fetches each dependency at the revision in `deps.lock.json` and builds Skia for the
host into `third_party/skia/out/<target>`. A Skia build from scratch takes about an hour;
later runs are skipped by a stamp file. `--with three` adds bgfx.

## Apps

An app is a directory with a `config.xf` (see [config-xf.md](config-xf.md)) and Prismio sources:

```
notes/
  config.xf        the one file a developer edits for app settings
  xefy.lock        resolved dependency versions
  src/             main.psm, app.psm, scope/, state/
  assets/          images/, fonts/, shaders/
  i18n/            en.xf, ...
  native/          optional C or C++ the app brings itself
  platform/        per-platform overrides, created only when needed
  tests/
  .xefy/           generated and gitignored: build.ums, bindings, caches, output
```

`config.xf` is the source of truth. The `xefy` tool generates `.xefy/build.ums` from it and
runs `prismio build`, so an app never states link flags or platform frameworks itself.

## AOS

- Apps are ordinary Linux ELF executables. Nothing about an app is freestanding.
- The `linux_drm` embedder drives the display through DRM/KMS and reads input through
  libinput, with no X server or Wayland compositor underneath. The system compositor and shell
  are a Xefy app on that embedder.
- Apps ship as `.xapp` bundles: the executable, assets, the manifest from `config.xf` and a
  signature. The OS reads `permissions` from the manifest at install time.
- Shipping the framework as one shared system library needs a stable Prismio ABI. Until that
  exists, each app links the framework statically.

## Milestones

| | Goal |
|---|---|
| M0 | Repository, setup tooling, Skia building on macOS and Linux |
| M1 | C ABI, Skia shim, macOS embedder, `xefy.gfx` drawing a frame from Prismio |
| M2 | Render tree: box and flex layout, paint, hit-test, text |
| M3 | Compose runtime and the first public API |
| M4 | Gestures, animation, text input, first `xefy.ui` composables |
| M5 | Linux Wayland and DRM embedders; `xefy build` and `xefy bundle` |
| M6 | AOS compositor and shell; `xefy.three` |

M2 and M3 depend on Prismio features that do not exist yet. They are listed with evidence, an
order of work and the workarounds Xefy uses meanwhile in `docs/XEFY_PLAN.md` of the Prismio
repository.
