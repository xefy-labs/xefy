# 0019. App state travels beside `scope` (v0)

Status: accepted (2026-11-12). Replaced with 0008 when Prismio can share state.

## Context
`Scope` can hold only values of types it knows (`Int`, `F32`, `Bool`, `String`), because Prismio
has no stored trait objects. An app's own data (a list of documents, a settings struct) cannot
live there. A content closure cannot capture that data either: capturing a struct moves it.

Probed on 0.2.0 and clean under `--verify`: a container that is generic over a state type takes
the state as a parameter and hands it to its content closure as a second parameter; fields are
assignable through that borrow.

## Decision
- An app keeps its data in one struct owned by `main` and passed down next to `scope`:
  `fn App(scope: Scope, state: AppState)`.
- Every container has a second form with four parameters, `(scope, state, classes, content)`,
  whose content closure is `|scope: Scope, state: AppState|`. The three-parameter form
  `(scope, classes, content)` stays for content that needs no app data.
- `runApp(config, state, root)` has the same two forms.
- `remember*` handles in `Scope` are for view state only: an open flag, a text field's draft, a
  selected tab.
- A function that changes app data calls `invalidate(scope)` so the next frame is drawn.

## Consequences
One state value is threaded through the tree; a subtree that needs only part of it is given
that field (`Sidebar(scope, state.projects)`). There is no observation: any change redraws the
whole frame, which v0 does anyway.
