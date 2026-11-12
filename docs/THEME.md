# Theme: Porcelain

Porcelain is the default theme and the reference for every other preset (decision 0022): a
preset supplies a value for each token below. These are the token values behind every
composable (decisions 0016 and 0018). `xefy.theme` is generated
from, or checked against, this file. Composables and class strings refer to tokens by the names
here; nothing else may hard-code a colour, size or duration.

Colours are `0xAARRGGBB` in code and written as `#RRGGBB` here. Sizes are logical pixels.

## Colour roles

| Role | Class name | Light | Dark | Used for |
|---|---|---|---|---|
| Background | `background` | `#F6F6F4` | `#141413` | The window behind everything |
| Surface | `surface` | `#FFFFFF` | `#1C1C1A` | Cards, panels, inputs |
| Raised surface | `surface-raised` | `#FFFFFF` | `#242422` | Popovers, menus, dialogs; light mode separates by shadow |
| Secondary | `secondary` | `#EEEDE9` | `#2A2A27` | Neutral button fill, selected row, hover wash |
| Text | `text` | `#1B1B1A` | `#EDEDEA` | Body and headings |
| Muted text | `muted` | `#6B6963` | `#A3A19A` | Supporting text, placeholders, icons at rest |
| Border | `border` | `#E4E2DC` | `#30302C` | Default hairline |
| Strong border | `strong` | `#CFCCC4` | `#45443F` | Hovered or emphasised hairline |
| Primary | `primary` | `#2F6BFF` | `#6E9BFF` | The main action, selection, links |
| Primary hover | `primary-hover` | `#1F5AEB` | `#8AAEFF` | |
| On primary | `on-primary` | `#FFFFFF` | `#0A1022` | Text and icons on a primary fill |
| Danger | `danger` | `#C5221F` | `#F28B82` | Destructive actions, errors |
| Success | `success` | `#1A7F45` | `#6CCB8E` | |
| Warning | `warning` | `#9A6200` | `#E5B455` | |
| Focus | `focus` | `#2F6BFF` | `#6E9BFF` | Focus ring |

Contrast ratios, computed from the values above (WCAG needs 4.5 for body text):

| Pair | Light | Dark |
|---|---|---|
| Text on background | 15.9 | 15.7 |
| Muted on background | 5.1 | 7.1 |
| Muted on surface | 5.5 | 6.6 |
| On-primary on primary | 4.5 | 7.0 |
| On-primary on primary-hover | 5.6 | 8.6 |
| Primary on surface (links) | 4.5 | 6.3 |
| Danger, success, warning on surface | 5.8, 5.0, 5.1 | 7.1, 8.6, 8.9 |

## Type

Family `Geist`; `Geist Mono` for `font-mono`. Desktop density, so the base size is 13.

| Class | Size | Line height | Default weight | Used for |
|---|---|---|---|---|
| `text-xs` | 11 | 16 | 400 | Captions, badges |
| `text-sm` | 12 | 18 | 400 | Secondary text, table cells |
| `text-base` | 13 | 20 | 400 | Body, controls |
| `text-lg` | 15 | 22 | 500 | Section headings |
| `text-xl` | 18 | 26 | 500 | Page headings |
| `text-2xl` | 22 | 30 | 600 | Titles |
| `text-3xl` | 28 | 36 | 600 | Display |
| `text-4xl` | 36 | 44 | 600 | Display |

Weights: `font-normal` 400, `font-medium` 500, `font-semibold` 600, `font-bold` 700.
Tracking: 0 up to 15 px, -0.01 em from 18 px, -0.02 em from 28 px.

## Space and size

| Token | Value |
|---|---|
| Spacing unit | 4. `p-1` is 4, `p-4` is 16 |
| Control height | 32 comfortable, 26 compact |
| Control horizontal padding | 12 comfortable, 8 compact |
| Icon size | 16 in controls, 20 standalone |
| Hairline | 1 |
| Focus ring | 2 wide, 2 outside the control, colour `focus` |
| Minimum pointer target | 24 |

## Shape

| Class | Radius | Used for |
|---|---|---|
| `rounded-sm` | 6 | Checkboxes, badges, menu items |
| `rounded-md` (and `rounded`) | 10 | Buttons, inputs, list rows |
| `rounded-lg` | 14 | Cards, popovers |
| `rounded-xl` | 20 | Dialogs, windows |
| `rounded-full` | half the height | Pills, avatars, switches |

## Elevation

Shadows are black at low opacity in light mode. In dark mode shadows are barely visible, so
raised surfaces use the lighter `surface-raised` fill and a `border` hairline as well.

| Class | Light | Dark | Used for |
|---|---|---|---|
| `shadow-sm` | y 1, blur 2, 6% | y 1, blur 2, 30% | Buttons, inputs |
| `shadow-md` | y 4, blur 12, 8% | y 4, blur 12, 40% | Cards on hover, popovers |
| `shadow-lg` | y 12, blur 32, 12% | y 12, blur 32, 50% | Dialogs, menus |

## Motion

| Token | Value |
|---|---|
| `duration-fast` | 120 ms: hover, press |
| `duration-normal` | 200 ms: the default for `transition` |
| `duration-slow` | 320 ms: dialogs, panels |
| `ease-standard` | cubic-bezier(0.2, 0, 0, 1) |
| `ease-spring` | spring, damping ratio 0.8, response 300 ms |

A setting to reduce motion replaces every transition with a 1-frame change.

## Composable states

How a control uses the roles, so every composable is consistent:

| State | Neutral control | Primary control |
|---|---|---|
| Rest | `surface` fill, `border` hairline, `text` | `primary` fill, `on-primary` text |
| Hover | `secondary` fill, `strong` hairline | `primary-hover` fill |
| Pressed | `secondary` fill, scale 0.98 | `primary-hover` fill, scale 0.98 |
| Focus | focus ring added, nothing else changes | the same |
| Disabled | 40% opacity, no hover, no pointer events | the same |
| Selected | `secondary` fill, `text` | `primary` fill |

## Shipped assets

| Asset | Licence | Where it goes |
|---|---|---|
| Geist (variable) | SIL OFL 1.1 | `framework/assets/fonts/` |
| Geist Mono (variable) | SIL OFL 1.1 | `framework/assets/fonts/` |
| Phosphor Icons, regular weight | MIT | `framework/assets/icons/` |
| Fallback family for other scripts | to be chosen | `framework/assets/fonts/` |

Each licence text goes in `licenses/`, and each asset is pinned by version and checksum in
`deps.lock.json`.
