# 0017. Utility class strings as the primary way to style

Status: accepted (2026-11-12)

## Context
Compose styles through `Modifier` chains. In Xefy v0 that is verbose (no named or default
arguments), and a chain cannot say "this colour on hover" without the app tracking hover
itself. Tailwind CSS showed that short utility names over a fixed token scale are fast to
write and keep a codebase visually consistent.

## Decision
- Every component takes an optional **class string**: `Text(ui, "Save", "text-sm font-medium")`,
  `Column(ui, "p-6 gap-3", |ui: Ui| { ... })`.
- A class is a utility over theme tokens (`p-4`, `bg-surface`, `rounded-lg`), optionally
  prefixed by variants for state, theme and window width (`hover:bg-primary`, `dark:`, `md:`).
  The vocabulary is in `docs/STYLE_CLASSES.md` and follows Tailwind's names wherever the
  concept exists in Xefy.
- A class string compiles to the same `ModifierOp` list a `Modifier` chain produces. There is
  one layout and paint path. `Modifier` stays for values computed at run time.
- This is not CSS: no cascade, no selectors, no inheritance, no stylesheet. A later class
  overrides an earlier one in the same string, and that is the only precedence rule.
- A string is parsed once and cached by its hash. Variants are evaluated per frame against
  the node's interaction state.
- An unknown class is reported: at run time once per string in debug builds, and statically
  by `xefy check`, which reads class-string literals from source.

## Consequences
- State styling and transitions (`hover:`, `pressed:`, `transition`) need no app code.
- Typos are not compile errors until Prismio can validate a string at compile time; the
  static check in the CLI covers literals only.
- The vocabulary is a public API: a class can be added, never repurposed.
