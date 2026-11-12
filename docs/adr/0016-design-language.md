# 0016. One Xefy design language, desktop first, with its own font and icons

Status: accepted (2026-11-12). The visual direction itself is chosen separately.

## Decision
- Components are named as in Jetpack Compose (`Column`, `Row`, `Box`, `Modifier`, `Scaffold`).
  Xefy invents a name only where Compose has no component.
- There is one Xefy look on every platform. Components do not imitate the host platform's
  controls. XefyOS uses the same look as its system UI.
- Desktop comes first: pointer and keyboard, hover states, visible focus, menus and context
  menus, resizable panes, dense lists and tables. Touch-only patterns (sheets, bottom
  navigation, swipe gestures) are tier 2.
- Xefy ships its own UI font, monospace font and icon set inside the framework, so an app
  looks the same everywhere and needs nothing installed. Only openly licensed assets (OFL,
  MIT, ISC, Apache 2.0) are shipped, with their licence texts in `licenses/`.
- The look is expressed only as theme tokens (`xefy.theme`). No component hard-codes a colour,
  radius, size or duration.

## Consequences
An app restyles by replacing tokens. Native menus and file dialogs are still the platform's on
macOS and Windows, because those are platform services rather than components.
