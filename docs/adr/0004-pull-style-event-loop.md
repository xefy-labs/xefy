# 0004. ABI v0 is pull-style: Prismio owns the loop

Status: accepted (2026-11-12)

## Context
Three Prismio 0.2.0 limits, all probed: a global holds only a literal, a closure cannot be
stored, and only a top-level function's address can be passed to C. A C callback into Prismio
therefore has nowhere to find the framework's state.

## Decision
- `main` owns the event loop. It calls `xefy_app_poll_event(app, &event, timeout_ms)` and
  receives one flat `xefy_event_t` (no unions, no pointers) per call.
- A frame is drawn on a `FRAME` event, between `xefy_window_begin_frame` and
  `xefy_window_end_frame`. `FRAME` arrives only for a window that requested one.
- v0 has no callbacks into Prismio at all.

## Consequences
- All framework state is a local in `main`, passed down by parameter (0008).
- macOS runs a modal loop during a live window resize, so a pull loop does not redraw until the
  drag ends unless the embedder works around it. Accepted for v0.
- Platforms that own `main` (Android, iOS) need callback delivery. It returns in a later MINOR
  once Prismio can store closures.
