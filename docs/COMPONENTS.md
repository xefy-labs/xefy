# Components

> Status: names, tiers and the look are decided (0016, 0018). Nothing here is built.

Names follow Jetpack Compose where Compose has the component, so documentation and habits
carry over. Signatures are in the v0 form (`docs/FRAMEWORK_V0.md`): `ui` first, then required
arguments, then an optional class string (`docs/STYLE_CLASSES.md`) or `Modifier`, then content.

Tiers: **0** is what the framework itself is tested with, **1** is what an ordinary app needs,
**2** is desktop and OS work that comes after v0.

## Layout

| Component | Tier | Notes |
|---|---|---|
| `Column`, `Row` | 0 | Flex layout; `spacing`, `weight`, main and cross alignment through `Modifier` |
| `Box` | 0 | Stacks children; `align` per child |
| `Spacer` | 0 | Fixed size, or flexible with `weight` |
| `Surface` | 0 | A `Box` with background, shape, border and elevation from the theme |
| `ScrollArea` | 0 | Vertical or horizontal scrolling of one child |
| `Divider` | 1 | Horizontal or vertical hairline |
| `FlowRow`, `FlowColumn` | 1 | Wrap to the next line when full |
| `Grid` | 1 | Fixed column count or minimum cell width |
| `LazyColumn`, `LazyRow`, `LazyGrid` | 1 | Compose only the visible items; needed for long lists |
| `SplitView` | 2 | Two panes with a draggable divider |

## Text and input

| Component | Tier | Notes |
|---|---|---|
| `Text` | 0 | Style from the theme's type scale; max lines and ellipsis |
| `TextField` | 1 | Single line: caret, selection, clipboard, undo |
| `TextArea` | 1 | Multi-line `TextField` |
| `SecureField` | 1 | Obscured text |
| `SearchField` | 1 | `TextField` with a leading icon and a clear button |
| `RichText` | 2 | Spans with mixed styles and links |

## Actions

| Component | Tier | Notes |
|---|---|---|
| `Button` | 0 | Returns true when activated. Variants: filled, tonal, outlined, text |
| `IconButton` | 1 | |
| `ToggleButton` | 1 | |
| `SegmentedControl` | 1 | One of several |
| `Link` | 1 | Inline text action |

## Selection

| Component | Tier | Notes |
|---|---|---|
| `Checkbox` | 0 | Takes a `Bool` state handle |
| `Switch` | 1 | |
| `RadioGroup` | 1 | |
| `Slider` | 1 | `F32` state handle, optional steps |
| `Stepper` | 1 | Number with minus and plus |
| `Select` | 1 | Dropdown choice; needs `Popover` |
| `DatePicker`, `TimePicker`, `ColorPicker` | 2 | |

## Containers and navigation

| Component | Tier | Notes |
|---|---|---|
| `Card` | 1 | `Surface` with standard padding and shape |
| `Tabs` | 1 | |
| `Scaffold` | 1 | Top bar, side bar, content, bottom bar slots |
| `TopBar` | 1 | |
| `Sidebar`, `NavigationRail` | 1 | Desktop navigation |
| `NavigationBar` | 2 | Bottom navigation for touch |
| `Dialog` | 1 | Modal, traps focus |
| `Popover` | 1 | Anchored floating surface; base for `Select`, `Menu`, `Tooltip` |
| `Menu`, `ContextMenu` | 1 | |
| `Tooltip` | 1 | |
| `Sheet`, `Drawer` | 2 | |
| `Accordion` | 2 | |

## Feedback

| Component | Tier | Notes |
|---|---|---|
| `ProgressBar` | 1 | Determinate and indeterminate |
| `Spinner` | 1 | |
| `Toast` | 1 | Short message that dismisses itself |
| `Badge` | 1 | |
| `Skeleton` | 2 | Placeholder while loading |

## Media and drawing

| Component | Tier | Notes |
|---|---|---|
| `Image` | 0 | From an asset path or bytes; fit modes |
| `Icon` | 1 | From an icon font or vector asset; tinted |
| `Canvas` | 0 | Custom drawing with `xefy.gfx` |
| `Scene3D` | 2 | The `three` feature (decision 0005) |
| `Video` | 2 | |

## Data

| Component | Tier | Notes |
|---|---|---|
| `List`, `ListItem` | 1 | Rows with leading, title, subtitle, trailing; selection |
| `Table` | 2 | Columns, sorting, resizable headers |
| `Tree` | 2 | Expandable hierarchy |

## Desktop and XefyOS

| Component | Tier | Notes |
|---|---|---|
| `MenuBar` | 2 | Application menu; maps to the native menu on macOS |
| `TitleBar` | 2 | Custom window chrome with drag region and window controls |
| `Toolbar` | 2 | |
| `StatusBar` | 2 | |
| Shell pieces: dock, launcher, notification centre, system tray | 2 | Part of `os/`, built from the components above |

## Theme

Porcelain (decision 0018). Every token value is in `docs/THEME.md`. Apps change the look by
replacing tokens, not by restyling components.
