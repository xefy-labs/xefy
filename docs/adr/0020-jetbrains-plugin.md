# 0020. A JetBrains plugin, in its own repository, that asks the `xefy` tool

Status: accepted (2026-11-12)

## Context
Prismio has a JetBrains plugin (`io.prismio`) that carries no copy of the language: everything
it knows comes from `prismio lsp`. Xefy adds things the Prismio plugin cannot know: `config.xf`,
class strings inside string literals, theme tokens, assets, and how to run an app.

## Decision
- A separate plugin, `io.xefy`, that **depends on `io.prismio`** and adds only Xefy's features.
  It never duplicates Prismio language support.
- It lives in its own repository (`xefy-labs/jetbrains-plugin`, checked out beside the SDK),
  written in Kotlin with the IntelliJ Platform Gradle plugin. This is outside decision 0002:
  the IntelliJ Platform runs on the JVM.
- **The plugin carries no copy of Xefy.** The class vocabulary, theme values, the `config.xf`
  schema and all diagnostics come from the `xefy` tool as JSON (`xefy ide ...` and
  `xefy check --format json`), so the plugin follows whatever SDK the project uses.
- The JSON contract is versioned and specified in `docs/IDE_PLUGIN.md`. The CLI owns it.

## Consequences
A new class or config setting needs no plugin release. The plugin is useless without an SDK,
and says so clearly when `xefy` is not found. A VS Code extension can be built on the same
contract later.
