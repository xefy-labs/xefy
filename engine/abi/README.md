# Xefy engine C ABI

`xefy_engine.h` is the single C header between the Prismio framework (layout,
painting, input handling) and everything native below it: the Skia shim and the
per-platform embedders (macOS, Linux Wayland/X11/DRM, Windows). The framework
only ever calls this header; embedders and the shim only ever implement it.

**Status: draft v0 (ABI 0.1). There is no implementation yet.** While MAJOR is
0 anything may still change. 3D/bgfx is out of scope and will get its own
optional header.

## Pull model: the framework owns the loop

ABI v0 is **pull-style**, like SDL or GLFW polling. Prismio's `main` creates the
app and windows, keeps all framework state in local variables, and repeatedly
asks the engine for the next event:

```c
xefy_event_t ev;
for (;;) {
  ev.struct_size = sizeof ev;
  if (xefy_app_poll_event(app, &ev, -1) != XEFY_OK) continue; /* XEFY_NO_EVENT on timeout */
  switch (ev.kind) {
    case XEFY_EVENT_FRAME: /* xefy_window_begin_frame, draw, xefy_window_end_frame */ break;
    /* ... */
  }
}
```

Why not callbacks? Prismio 0.2.0 has three hard limits that make a
callback-driven design unusable from it today: a global variable can only hold
a literal (so there is nowhere to keep framework state that a C callback could
reach), closures cannot be stored, and only top-level functions can be passed
to C. A pull loop needs none of these: state lives in `main`'s locals and the
engine never calls back.

- `xefy_app_poll_event(app, &ev, timeout_ms)`: `0` = do not block, `-1` = wait
  for an event, `>0` = wait at most that long. Returns `XEFY_OK` with an event,
  or `XEFY_NO_EVENT` (a non-error status, value 100) when the timeout expires.
- `xefy_app_wake(app)` may be called from any thread and makes a blocked poll
  return a `WAKE` event; this is how background work gets the loop's attention.
- There is no `run`, `on_ready`, `on_will_quit` or posted-task queue. Quitting
  is the loop's decision: it receives `QUIT_REQUESTED` (from the OS or from
  `xefy_app_quit`) and stops polling. A window is closed by the framework
  calling `xefy_window_destroy` after `WINDOW_CLOSE_REQUESTED`; ignoring the
  event vetoes the close.
- Callback delivery will come back in a later MINOR for platforms that own
  `main` (Android, iOS), once Prismio can store closures.

### Drawing

Windows are identified in events by a `uint32_t window_id`
(`xefy_window_get_id`). `xefy_window_request_frame` asks for one `FRAME` event
at the next vsync. The intended use is: on `FRAME`, `xefy_window_begin_frame`
(gives the canvas), draw, `xefy_window_end_frame` (flushes and presents), and
request the next frame if animating. Canvas handles are valid only between
begin and end; canvas calls outside return `XEFY_ERR_WRONG_STATE`.

### Why one flat event struct

`xefy_event_t` is a single flat record: it starts with `uint32_t struct_size`,
holds only `int32_t`/`uint32_t`/`int64_t`/`uint64_t`/`float` fields, and has no
unions, nested structs, arrays or pointers. Prismio reads it through a
`repr(C)` struct and has no unions, so a per-kind union would be unreadable
there. The cost is some unused space: fields that do not belong to the event's
`kind` are zero. Explicit padding keeps every 64-bit field 8-byte aligned
(current size: 104 bytes, a multiple of 8). The one variable-length payload,
committed text for `TEXT_INPUT`, is not in the struct: fetch it with
`xefy_app_get_text_input` (caller buffer, returns the byte length, same
convention as the clipboard).

Event kinds: `NONE`, `QUIT_REQUESTED`, `WAKE`, `WINDOW_CLOSE_REQUESTED`,
`WINDOW_RESIZED`, `WINDOW_SCALE_CHANGED`, `WINDOW_FOCUS`, `FRAME`, `POINTER`,
`KEY`, `TEXT_INPUT`.

## Rules and why they exist

These follow from current Prismio `extern fn` limits and from keeping every
platform implementation trivially bindable.

1. **Plain C99, only `<stdint.h>`/`<stddef.h>`, `extern "C"`.** Any compiler,
   any FFI, no C++ ABI or STL in the contract.
2. **No struct by value.** Prismio cannot pass or return aggregates; structs
   cross as `const T*` / `T*`.
3. **No integer narrower than 32 bits** (booleans are `int32_t` 0/1). Small
   ints are unreliable in the FFI; struct fields carry explicit padding.
4. **`float` geometry, `int64_t` nanosecond time.** One numeric convention for
   all platforms; one monotonic clock for every event timestamp.
5. **Strings are `utf8 + length_bytes`**, never NUL-dependent. Output strings
   go into a caller buffer and the call returns the needed length, so the
   engine never allocates memory the caller must free.
6. **v0 has no callbacks.** No function pointers anywhere in the header; the
   engine reports everything through `xefy_app_poll_event`. Callbacks return in
   a later MINOR for platforms that own `main`.
7. **Opaque handles with explicit create/destroy** and documented ownership
   (the canvas is borrowed, valid only between `begin_frame` and `end_frame`).
8. **`xefy_` prefix, `XEFY_API` export, `int32_t` status codes.** No exception
   or longjmp ever crosses the boundary. Errors are 1..99; codes >= 100
   (`XEFY_NO_EVENT`) are non-error results.
9. **Versioned.** `XEFY_ENGINE_ABI_MAJOR/MINOR`, `xefy_engine_abi_version()`,
   and growable structs begin with `uint32_t struct_size`.
10. **Threading is single-thread.** Everything, including `xefy_app_poll_event`
    and all drawing, runs on the thread that created the app; other threads get
    `XEFY_ERR_WRONG_THREAD`. Only `xefy_app_wake`, `xefy_app_quit` and
    `xefy_engine_abi_version` may be called from any thread.
    `xefy_thread_attach/detach` are placeholders for future raster/vsync threads.

## Evolving the ABI

- Additive change (new function, new enum value, new field appended to a
  `struct_size`-prefixed struct such as `xefy_event_t`, new event kind): bump
  **MINOR**.
- Breaking change (removed/renamed function, changed signature, changed enum
  value, reordered/resized field): bump **MAJOR**. The engine rejects a config
  whose `abi_version` major differs (`XEFY_ERR_ABI_MISMATCH`).
- Never reuse a removed enum value or struct slot; consumers must treat unknown
  enum values (including unknown event kinds) as unsupported and ignore struct
  bytes beyond what they know.
- Check any edit with `clang -std=c99 -Wall -Wextra -Wpedantic -fsyntax-only
  -x c engine/abi/xefy_engine.h` and the `clang++ -std=c++17` equivalent.
