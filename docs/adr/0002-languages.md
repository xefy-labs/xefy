# 0002. Languages: Prismio above the boundary, C++ below it, Python for scripts

Status: accepted (2026-11-12)

## Context
Xefy exists partly to prove Prismio on a large program. Skia is a C++ library and platform
windowing APIs are Objective-C, C++ or C.

## Decision
- Everything above `engine/abi/xefy_engine.h` is Prismio: layout, painting logic, input
  handling, text editing, state, composables, the CLI.
- C++ and Objective-C++ are allowed only under `engine/` and `vendor/`, and only to reach an
  API that cannot be reached from C.
- Repository scripts are Python 3.9, standard library only. No shell scripts, no Makefiles.
- An app may bring its own C through `native {}` in `config.xf`; the framework does not.

## Consequences
When Prismio cannot express something, the answer is a row in the Prismio repository's
`docs/XEFY_PLAN.md` and an interim design in Prismio, not a C helper. A reviewer rejects native
code that could have been Prismio.
