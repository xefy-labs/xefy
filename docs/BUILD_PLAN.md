# Build plan

Work packages for getting from the scaffold to a working framework v0. Each package is sized
for one agent in one session, owns a fixed set of paths, and ends in a check that can be run.
Packages with no dependency between them can run in parallel.

## Rules for every package

1. **Read first:** `docs/ARCHITECTURE.md`, the decision records named in the package, and
   `docs/FRAMEWORK_V0.md` for anything under `framework/`.
2. **Stay in your paths.** Create and edit only what the package lists under *Owns*. If another
   path needs a change, stop and report it; do not make it.
3. **Prismio is not a language you know.** Its reference is https://docs.prismio.org (the
   `language/`, `stdlib/` and `guides/` sections). Its style guide is `CODE_STYLE.md` and its
   known gaps are `docs/XEFY_PLAN.md`, both in the Prismio repository
   (https://github.com/prismio-lang/prismio); use a local checkout when there is one. Probe a construct with a small file
   and `prismio check` before building on it. Every `.psm` file passes `prismio fmt --check`.
4. **No native code above the ABI** and no shell scripts (0002). Python is 3.9, standard
   library only.
5. **A port names its source.** A file translated from Flutter starts with a comment giving
   the upstream path, the revision from `deps.lock.json`, and what changed (0007).
6. **Do not weaken a check to make it pass.** If the *Done when* cannot be met, report what
   fails and why.
7. **Report:** files written, the real output of each check, and anything implemented but not
   run. If you find a Prismio compiler bug, give a minimal reproduction.
8. **No commits.**

Model column: H = Haiku is enough, S = Sonnet, O = needs the strongest model or a human review
of the design before code.

## Phase A: engine

| # | Package | Owns | Depends on | Model |
|---|---|---|---|---|
| A1 | **Binding generator.** Python script that reads `xefy_engine.h` and writes `framework/xefy/ffi.psm`: one `extern fn` per function with ownership contracts, `repr(C)` structs, constants for every enum value. Wire it as `python3 xefy.py bindings`, with `--check` that fails when the output is stale | `tools/xefy_tools/bindings.py`, `framework/xefy/ffi.psm` | header | S |
| A2 | **Engine build.** `python3 xefy.py build-engine` compiles everything under `engine/` for the host into `build/<target>/libxefy_engine.a`, with include paths for Skia, incremental by file hash. Prints the link line an app needs (Skia libraries, frameworks) and writes it to `build/<target>/link.json` | `tools/xefy_tools/engine.py` | setup tooling | S |
| A3 | **Skia shim: drawing.** Implement the canvas, paint, path and image functions of the header on Skia. Add an offscreen raster surface to the header (create with width, height and scale; get its canvas; write PNG to a path) for tests. A C test draws shapes to a PNG and checks pixels | `engine/skia_c/*` except text files, `engine/abi/xefy_engine.h` (offscreen functions only), `tests/engine/draw_*` | A2 | S |
| A4 | **Skia shim: text.** Font collection, paragraph builder, paragraph layout, metrics, hit-testing, range and caret rectangles, grapheme boundaries, with UTF-8 byte offsets at the boundary (0011). A C test lays out Latin, Devanagari and emoji text and checks metrics and offsets | `engine/skia_c/text_*`, `tests/engine/text_*` | A2 | S |
| A5 | **macOS embedder.** App, window, the pull event loop, pointer, key and text events, vsync `FRAME` events, `begin_frame`/`end_frame` on a Metal-backed Skia surface, clipboard, cursor. A C example opens a window, draws an animated rectangle and logs events | `engine/embedder/macos/*`, `examples/c_window/*` | A2, A3 | O |
| A6 | **Linux Wayland embedder**, same surface as A5, plus validating `tools/skia_args/linux-*.gn` on a Linux machine | `engine/embedder/linux_wayland/*`, `tools/skia_args/linux-*.gn` | A3, a Linux host | O |

Done when, for A3 to A5: the C tests or example build through `python3 xefy.py build-engine`
and run; the header still compiles as C99 and C++17 with `-Wall -Wextra -Wpedantic`; and
`python3 xefy.py bindings --check` passes.

## Phase B: framework foundations (no window needed)

| # | Package | Owns | Depends on | Model |
|---|---|---|---|---|
| B1 | **`xefy.geometry`** with `Constraints` ported from Flutter's `BoxConstraints`, and tests | `framework/xefy/geometry.psm`, `framework/tests/geometry_test.psm` | none | H |
| B2 | **`xefy.color`** and tests | `framework/xefy/color.psm`, test | none | H |
| B3 | **`xefy.animation`**: extend the cubic curve port already in `framework/xefy/animation.psm`: add the rest of Flutter's named curves, linear tweens for `F32` and colour, and Flutter's spring simulation (`physics/spring_simulation.dart`) | `framework/xefy/animation.psm`, test | B2 | S |
| B4 | **`xefy.modifier`** | `framework/xefy/modifier.psm`, test | B1, B2 | H |
| B8 | **`xefy.style`**: the class-string parser, the cache, variant resolution against a state record, and every utility in `docs/STYLE_CLASSES.md`. Tests for each utility, each variant, override order and every error case | `framework/xefy/style.psm`, `framework/tests/style_*` | B4, C5 | S |
| B5 | **`xefy.tree`**: arena, node kinds and data, ids, child linking, a debug dump | `framework/xefy/tree.psm`, test | B4 | S |
| B6 | **`xefy.layout`**: modifiers, flex (port `RenderFlex`), box (port `RenderStack`), spacer, image, scroll, text through a replaceable measurer. Port Flutter's flex tests | `framework/xefy/layout.psm`, `framework/tests/layout_*` | B5 | S, design review O |
| B7 | **Framework test runner.** `python3 xefy.py test` builds and runs every `framework/tests/*_test.psm` and reports failures | `tools/xefy_tools/testing.py`, `framework/build.ums` | none | S |

Done when: `python3 xefy.py test` passes and every file passes `prismio fmt --check`.

## Phase C: the runtime

| # | Package | Owns | Depends on | Model |
|---|---|---|---|---|
| C1 | **`xefy.gfx` and `xefy.platform`**: wrappers over `xefy.ffi`. Example: a Prismio program that opens a window and draws shapes and a paragraph | `framework/xefy/gfx.psm`, `platform.psm`, `examples/hello_canvas/*` | A1, A3, A4, A5 | S |
| C2 | **`xefy.ui`**: `Ui`, state pools and slot keys, the input snapshot, retained per-id data. Tests with scripted input | `framework/xefy/ui.psm`, test | B5 | S, design review O |
| C3 | **`xefy.hit`**: hover, press, activation, focus, keyboard activation, scroll targeting. Tests | `framework/xefy/hit.psm`, test | C2 | S |
| C4 | **`xefy.paint`** and paragraph caching | `framework/xefy/paint.psm` | B5, C1 | S |
| C5 | **`xefy.theme`**: the light and dark token sets from `docs/THEME.md`, with a test that recomputes the contrast table | `framework/xefy/theme.psm`, test | B2 | H |
| C8 | **Shipped assets.** Pin Geist, Geist Mono and Phosphor (regular) in `deps.lock.json` by version and checksum, fetch them in `setup` into `framework/assets/`, add their licence texts, and an icon-name table for `Icon` | `tools/xefy_tools/assets.py`, `deps.lock.json`, `framework/assets/*`, `licenses/*` | none | S |
| C6 | **`xefy.components`, tier 0** (see `docs/COMPONENTS.md`) and `remember*`, `key` | `framework/xefy/components.psm`, tests | C2, C3, C5 | S |
| C7 | **`xefy.app`**: `runApp` and the frame loop. Example: the counter from `docs/FRAMEWORK_V0.md` | `framework/xefy/app.psm`, `examples/counter/*` | C1 to C6, B6 | S |

Done when: `examples/counter` builds, opens a window, and the count rises on click and on
Space with the button focused; component tests pass headless.

## Phase D: components and tools

| # | Package | Owns | Depends on | Model |
|---|---|---|---|---|
| D1 | **Text field.** Single-line and multi-line editing: caret, selection by keyboard and pointer, word movement, clipboard, undo. Behaviour ported from Flutter's editable text | `framework/xefy/text_field.psm`, tests | C7, A4 | O |
| D2 | **Scrolling.** Scroll node behaviour, wheel and drag, scrollbar, Flutter's clamping and bouncing physics | `framework/xefy/scroll.psm`, tests | C7, B3 | S |
| D3 | **Components, tier 1**, one package per group in `docs/COMPONENTS.md` | `framework/xefy/components_<group>.psm` | C7 | S each |
| D4 | **`config.xf` parser** in the CLI, with every error case in `docs/config-xf.md` tested | `cli/src/config*.psm`, `cli/tests/*` | none | S |
| D5 | **`xefy build` and `xefy run`**: generate `.xefy/build.ums` from `config.xf` and `build/<target>/link.json`, link the framework symlink (0013), call `prismio build` | `cli/src/build*.psm` | D4, A2 | S |
| D6 | **`xefy bundle` for macOS**: `.app` with `Info.plist`, icon and assets | `cli/src/bundle*.psm` | D5 | S |
| D9 | **`xefy check`**: scan class-string literals in an app's sources and report unknown or misapplied classes with file and line | `cli/src/check*.psm` | B8 | S |
| D7 | **Gallery example** showing every component, used for manual checks and screenshots | `examples/gallery/*` | D3 | S |
| D8 | **Golden image tests**: render component states offscreen and compare with stored PNGs | `tools/xefy_tools/golden.py`, `framework/tests/golden/*` | A3, C7 | S |

## Order

```
A1  A2 ─┬─ A3 ─┬─ A5 ──┐
        └─ A4 ─┘       ├─ C1 ─ C4 ─┐
B1 B2 ─ B4 ─ B5 ─ B6 ──┤           ├─ C7 ─ D1 D2 D3 D7 D8
B3  B7        └─ C2 ─ C3 ─ C6 ─────┘
              C5 ──────────┘
D4 ─ D5 ─ D6
```

Phase B and D4 need nothing from the engine and can start now. A5 is the package most likely
to need a second pass.

## Not planned here

Windows, X11 and DRM embedders, `xefy.three`, XefyOS, accessibility, and the move to the
target Compose API. Each gets its own plan once v0 runs.
