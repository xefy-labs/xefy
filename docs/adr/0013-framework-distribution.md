# 0013. How an app sees the framework

Status: accepted (2026-11-12). Interim until Prismio resolves package dependencies.

## Context
Prismio 0.2.0 resolves imports only below the directory of the entry file, and cannot yet
consume a third-party compiled library.

## Decision
- The framework is plain Prismio source under `framework/xefy/`, one file per module:
  `framework/xefy/geometry.psm` is `import xefy.geometry`.
- An app (and each example) gets a symlink `src/xefy` pointing at `framework/xefy`, created by
  `xefy create` and refreshed by `xefy build`. It is gitignored in the app.
- The engine is linked as static libraries named in the generated `build.ums`.
- Imports are not transitive and there are no re-exports, so an app imports each module it uses.

## Consequences
Every app compiles the framework from source. That is slow but needs nothing from the
language. When Prismio gains package dependencies the symlink is replaced by a dependency entry
and nothing in app source changes.
