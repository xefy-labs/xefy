# 0022. Themes are configurable; Porcelain is the default

Status: accepted (2026-11-12). Amends 0016 and 0018.

## Context
0016 said there is one Xefy look. The intent is one set of composables on every platform; the
look itself should be the app's choice, with a good default and ready-made alternatives.

## Decision
- A **theme** is a complete set of the tokens named in `docs/THEME.md`, for light and dark.
  Composables read tokens only, so any theme works with every composable and every tint class.
- Xefy ships theme **presets**. Porcelain is the default and the reference: its values are in
  `docs/THEME.md`. Further presets (Graphite and Signal first) are token files with the same
  roles and the same contrast checks.
- An app picks and adjusts its theme in `config.xf`:
  ```
  theme {
      preset = "porcelain"
      mode   = "system"          // "light", "dark" or "system"
      file   = "theme.xf"        // optional: the app's own token overrides
  }
  ```
  `theme.xf` uses the same grammar and may override any token or define a whole theme.
- `xefy create --theme <preset>` writes the chosen preset into the new app's `config.xf`.
- The `xefy` tool resolves preset plus overrides at build time into a generated Prismio module,
  so the framework parses nothing at run time. Light and dark are both generated, and the app
  can switch mode while running.
- AOS's own shell uses Porcelain.

## Consequences
A preset is data, so adding one needs no code. A theme that fails the contrast table in
`docs/THEME.md` is rejected by `xefy check`. A look that needs something tokens cannot express
(backdrop blur for the Aurora direction) waits until a token and engine support for it exist.
