# 0006. A Compose-style API over a Flutter-derived layout and paint model

Status: accepted (2026-11-12)

## Context
The UI should be written like Jetpack Compose. Flutter's `rendering/` library is the most
proven open implementation of constraint-based layout, and its licence allows reuse.

## Decision
- The public API follows Compose: functions named `Column`, `Row`, `Box`, `Text`; `Modifier`
  chains; `remember` and state; content as trailing blocks.
- Layout, painting and hit-testing follow Flutter's box protocol: constraints go down, sizes
  come up, the parent positions the child. Flex, stack, padding, alignment, scrolling and text
  layout are ported from Flutter's Dart sources into Prismio.
- Flutter's `widgets/`, `material/` and `cupertino/` are not ported.

## Consequences
The target API needs Prismio features that do not exist (stored closures, trailing closures,
named arguments, a composable transform). Until they do, the framework ships the v0 API in
0008, which keeps the component names and the layout model and changes only how context, events
and state are spelled.
