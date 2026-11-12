# 0001. Repository layout and the two tools

Status: accepted (2026-11-12)

## Context
The project holds a native engine, a Prismio framework, a CLI, an OS and a website. Third-party
sources are large (Skia is 9.5 GB) and the first prototype mixed them with Xefy's own code.

## Decision
- `Xefy/xefy/` is the SDK and the git repository. `Xefy/website/` sits beside it and is not part of it.
- Top-level directories are fixed: `tools/`, `third_party/` (fetched, gitignored), `vendor/`
  (copied, checked in), `engine/`, `framework/`, `cli/`, `templates/`, `os/`, `examples/`,
  `tests/`, `docs/`, `licenses/`.
- Two tools, two audiences: `python3 xefy.py` prepares and builds this repository; the `xefy`
  binary from `cli/` creates and builds apps.
- Dependencies are pinned by full commit hash in `deps.lock.json`. Nothing is fetched at any
  other revision.

## Consequences
A new top-level directory needs a decision record.
