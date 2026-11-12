# What the OS needs from Xefy

AOS is a desktop OS on the Linux kernel whose
shell and first-party apps are Xefy apps. Its repository holds the OS design
(`docs/DESIGN.md`), the component list (`docs/STACK.md`) and its own plan (`docs/PLAN.md`).
This document lists what that plan needs from Xefy that `BUILD_PLAN.md` does not cover, as
work packages in the same form. It assumes decision 0023.

The rules for every package in `BUILD_PLAN.md` apply here too.

## Gaps

| Need | In the current plan? |
|---|---|
| Linux Wayland embedder | A6, as a plain window only |
| Skia built for Linux on aarch64 and x86_64, GPU and software | Part of A6 |
| Layer-shell surfaces (bar, dock, overlays anchored to screen edges) | No |
| More than one surface per process, one per monitor, with per-monitor scale | ABI has windows; outputs are missing |
| Waiting on foreign file descriptors in the pull loop | No. `xefy_app_poll_event` waits only on engine events |
| D-Bus from Prismio | No |
| Window list and activation for a dock and switcher | No |
| IME composition (preedit) | Listed as a TODO in the header; v0 delivers committed text only |
| Clipboard beyond plain text, drag and drop | No |
| Accessibility tree | Listed under "Not planned here" |
| Following the system colour scheme, accent and scale | No |
| File and other system dialogs through portals | No |
| Reading `.desktop` entries and icon themes | No |
| Packaging an app as a Flatpak | No. `xefy bundle` is macOS only (D6) |
| A shared engine library | 0015 defers it to a stable Prismio ABI, but the engine side has a stable C ABI already |
| Partial redraw and idle at zero CPU | Frames are on request (good); damage tracking is not planned |
| Session lock surface | No |
| Work off the UI thread | 0014 defers it; a shell must not freeze on a slow D-Bus call |

Not needed any more: the `linux_drm` embedder, a compositor, a private surface protocol and a
`.xapp` sandbox (0023).

## Phase F: Linux platform

Starts after A3 and A4. F1 replaces A6.

| # | Package | Owns | Depends on | Model |
|---|---|---|---|---|
| F1 | **Wayland embedder, windows.** Everything A5 does, on Wayland: xdg-shell toplevel, the pull loop over the Wayland fd, pointer, keyboard through xkbcommon, key repeat, frame callbacks as `FRAME` events, fractional-scale and viewporter, cursor-shape, xdg-decoration (request server-side), text clipboard, Skia on Vulkan with a GL fallback and a software fallback for VMs. Validates `tools/skia_args/linux-*.gn` on aarch64 and x86_64 | `engine/embedder/linux_wayland/*`, `tools/skia_args/linux-*.gn` | A2, A3, a Linux host | O |
| F2 | **File descriptor sources.** ABI: `xefy_app_add_fd(app, fd, events, out_source_id)`, `xefy_app_remove_fd`, and an `XEFY_EVENT_FD_READY` event carrying the source id. Implemented in every embedder | `engine/abi/xefy_engine.h` (these functions), each embedder's loop | A5 or F1 | S |
| F3 | **Outputs.** ABI: enumerate outputs with name, logical geometry, scale and refresh; `OUTPUT_ADDED/REMOVED/CHANGED` events; which output a window is on | header (output functions), `linux_wayland`, `macos` | F1 | S |
| F4 | **Layer surfaces.** ABI: create a surface on a layer (background, bottom, top, overlay) of a given output with anchors, margins, exclusive zone and keyboard mode. Implemented with wlr-layer-shell; returns `XEFY_UNSUPPORTED` elsewhere. Example: a 32-pixel bar on every output under KWin | header (layer functions), `linux_wayland/layer_*`, `examples/c_bar/*` | F1, F3 | O |
| F5 | **IME composition.** Preedit event with text and cursor range, through text-input-v3; the macOS embedder gets the same event | header (preedit event), both embedders | F1, D1 | S |
| F6 | **Clipboard MIME types and drag and drop** | header (data functions), both embedders | F1 | S |
| F7 | **Damage tracking.** The framework reports dirty rectangles per frame; the embedder passes them to the compositor and Skia redraws only those | `framework/xefy/paint.psm`, embedders | C4, F1 | S, design review O |
| F8 | **Session lock surface** through ext-session-lock | header, `linux_wayland` | F4 | S |

Done when, for F1 to F4: `examples/counter` and `examples/c_bar` run on the AOS image under
KWin, on a HiDPI output, with the software renderer in a VM and a GPU on hardware.

## Phase G: system integration (Prismio packages)

| # | Package | Owns | Depends on | Model |
|---|---|---|---|---|
| G1 | **`xefy.dbus`.** Binding over sd-bus: connect to the session and system bus, call a method, read and watch properties, subscribe to signals, own a name and export an object. The bus fd is registered through F2; no thread | `framework/xefy/dbus.psm`, a small C shim only if sd-bus's varargs calls cannot be declared from Prismio, tests against a private bus | F2 | O |
| G2 | **`xefy.system`** typed clients over G1: NetworkManager, BlueZ, UPower, logind, power profiles, MPRIS, notifications server, status notifier host | `framework/xefy/system/*.psm` | G1 | S each |
| G3 | **`xefy.desktop`**: parse `.desktop` entries and icon themes (freedesktop specifications), launch apps through systemd user scopes | `framework/xefy/desktop.psm`, tests | none | S |
| G4 | **Window list.** List, activate, close and watch windows for a dock and switcher, over the protocol the compositor offers. Preceded by spike P2.1 in the OS plan, which decides the protocol | header (toplevel functions) or a Prismio protocol client, tests | F1, spike | O |
| G5 | **Portals**: file chooser, open URI, screenshot, and the Settings portal for colour scheme, accent and contrast, feeding `xefy.theme` | `framework/xefy/portal.psm` | G1, C5 | S |
| G6 | **Accessibility.** `xefy.semantics` builds a tree; the engine hands it to AccessKit (C bindings), which speaks AT-SPI on Linux and the native APIs on macOS and Windows | header (semantics functions), `engine/skia_c` or a new `engine/a11y`, `framework/xefy/semantics.psm` | C6 | O |
| G7 | **Background work** once Prismio has synchronisation types: run a blocking call on a worker and deliver the result as an event | `framework/xefy/task.psm` | Prismio | O |

## Phase H: packaging

| # | Package | Owns | Depends on | Model |
|---|---|---|---|---|
| H1 | **`xefy bundle` for Linux**: a Flatpak with `.desktop` file, AppStream metadata and icon generated from `config.xf`; `permissions` mapped to Flatpak permissions | `cli/src/bundle_flatpak*.psm` | D5 | S |
| H2 | **Shared engine.** Build `libxefy_engine.so` with Skia inside and ship it in an AOS Flatpak runtime, so apps link the Prismio framework statically but share the engine | `tools/xefy_tools/engine.py`, runtime manifest | A2, H1 | S |

## Phase S: the shell (in the OS repository, `shell/`)

Each is a Xefy app and a systemd user service. Listed here because they are the acceptance
test for phases F and G.

| # | App | Needs |
|---|---|---|
| S1 | Launcher and search | F1, G3, D1 |
| S2 | Settings | F1, G1, G2, D3 |
| S3 | Top bar | F4, G2 |
| S4 | Dock | F4, G3, G4 |
| S5 | Notifications and control centre | F4, G2 |
| S6 | Lock screen | F8 |

## Order

```
BUILD_PLAN A to D (framework v0)
        │
A3 A4 ─ F1 ─┬─ F2 ─ G1 ─ G2 ─┬─ S2
            ├─ F3 ─ F4 ──────┼─ S3 S5
            │        └─ F8 ─ S6
            ├─ G4 ───────────┴─ S4
            └─ F5 F6 F7
G3 ─ S1          G5 G6 H1 H2 in parallel once C7 is done
```

G3 and the spike before G4 need nothing from the engine and can start now.

## Requests to Prismio

To be added to `docs/XEFY_PLAN.md` in the Prismio repository:

1. Declaring and calling C variadic functions, or G1 needs a C shim.
2. Stored closures or an equivalent, so D-Bus signal handlers can be registered instead of
   matched in one large `match`.
3. Synchronisation types and a worker thread (G7).
4. A stable ABI for a shared framework library; until then H2 shares only the engine.
