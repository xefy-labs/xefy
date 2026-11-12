# Prototype macOS host (reference only)

`platform_macos.cpp` and `xefy_platform.h` are the window host from the first Xefy prototype:
an AppKit window with a Metal-backed Skia surface, keyboard delivery and a small draw API. It
worked, and it is the only code in this repository that has opened a window and drawn through
Skia.

It is kept as reading material for the macOS embedder (package A5 in `docs/BUILD_PLAN.md`).
Nothing builds it, and its design is superseded: it calls back into Prismio and routes events
by integer id, where the engine ABI is now pull-style (decision 0004). Delete this directory
once `engine/embedder/macos/` exists.
