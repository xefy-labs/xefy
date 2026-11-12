# 0012. `config.xf` is an app's source of truth

Status: accepted (2026-11-12)

## Decision
- Each app has one `config.xf` (specified in `docs/config-xf.md`) in the block grammar of
  Prismio's `build.ums`.
- The `xefy` tool generates everything else under `.xefy/`: `build.ums`, generated bindings,
  the asset manifest, bundle metadata. `.xefy/` is never edited and never committed.
- An app overrides a generated platform file only by placing one under `platform/<target>/`.
- Unknown blocks and settings are errors.

## Consequences
Developers do not write link flags or framework lists. The `config.xf` parser lives in `cli/`
in Prismio and is the only reader of the format; nothing else parses it with regular expressions.
