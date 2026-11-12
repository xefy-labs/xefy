# 0021. Names

Status: accepted (2026-11-12). Applied to every document on the same day, including earlier
records, so that the text agrees everywhere.

## Decision
| Thing | Name |
|---|---|
| The declarative UI layer, in documentation and marketing | **Xefy UI**. Not "Compose", which is Google's and JetBrains' product name |
| A UI function (`Column`, `Text`, an app's own `ProjectCard`) | **composable**. Re-running them is **recomposition**. The future compiler marker is `@Composable` |
| The context every composable receives first | `scope: Scope` |
| The module of built-in composables | `xefy.ui` |
| The module holding `Scope`, state and the frame lifecycle | `xefy.runtime` |
| Utility class strings | **Tint**. Module `xefy.tint`; `tint("p-4")` turns a string into a `Modifier`; one item is a tint class; the document is `docs/TINT.md` |
| The default theme | **Porcelain** (see 0022) |
| Fixed earlier and unchanged | the `xefy` tool, `config.xf`, `.xapp` bundles, AOS, composable names taken from Jetpack Compose |

## Consequences
- `scope` is also the word Prismio uses for its structured-concurrency block (`scope { }`).
  A parameter named `scope` compiles today (probed on 0.2.0), but a function that takes one
  cannot open a `scope { }` block without the two colliding. If that becomes a problem the
  parameter is renamed here, in one record.
- "Component" is no longer used for UI functions anywhere in Xefy's documentation, error
  messages, IDE plugin or templates.
