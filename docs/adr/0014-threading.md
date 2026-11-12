# 0014. One thread in v0

Status: accepted (2026-11-12)

## Decision
The event loop, composition, layout, paint and Skia rasterisation all run on the thread that
called `xefy_app_create`. The engine may use its own internal threads (vsync source, image
decode) but never calls into Prismio from them; it turns their results into events for
`xefy_app_poll_event`, and `xefy_app_wake` is the only cross-thread entry point.

## Consequences
No locks in the framework. Long work in app code blocks the UI; v0 has no answer to that beyond
"do it in small steps across frames". A raster thread and async effects wait for Prismio's
synchronisation types and a yielding `await`.
