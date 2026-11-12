# Style classes

The vocabulary of class strings (decision 0017). Names follow Tailwind CSS where Xefy has the
same concept, so what a web developer already knows works. Where Xefy differs, this file says so.

```prismio
fn ProjectCard(ui: Ui, name: String, summary: String) {
    Column(ui, "p-4 gap-2 bg-surface rounded-lg border hover:border-strong transition", |ui: Ui| {
        Text(ui, name, "text-base font-medium")
        Text(ui, summary, "text-sm text-muted line-clamp-2")
        Row(ui, "gap-2 justify-end", |ui: Ui| {
            if Button(ui, "Open", "bg-primary text-on-primary hover:bg-primary-hover") { }
        })
    })
}
```

## Grammar

```
classes  = class (" " class)*
class    = (variant ":")* utility
utility  = name ("-" value)? ("/" opacity)?
value    = token | number | fraction | "[" arbitrary "]"
```

- Classes apply left to right; a later class overrides an earlier one that sets the same thing.
- `[...]` is an arbitrary value: `w-[320]` (logical pixels), `bg-[#7D73FF]`, `rounded-[14]`.
- `/` after a colour is opacity in percent: `bg-primary/50`.
- A leading `-` negates a numeric value where that makes sense: `-mt-2` is not supported
  (there are no margins); `-translate-x-2` is.

## Scales

| Scale | Values |
|---|---|
| Spacing `n` | `n` x 4 logical pixels: `0 0.5 1 1.5 2 3 4 5 6 8 10 12 16 20 24`, plus `px` = 1 |
| Radius | `none sm md lg xl full` from the theme |
| Text size | `xs sm base lg xl 2xl 3xl 4xl` from the theme's type scale |
| Shadow | `sm md lg` from the theme |
| Colour | Theme roles: `background surface surface-raised primary primary-hover on-primary secondary text muted border strong danger success warning focus`. Arbitrary colours through `[#RRGGBB]` |

## Utilities

| Group | Classes |
|---|---|
| Padding | `p-n px-n py-n pt-n pr-n pb-n pl-n` |
| Gap between children | `gap-n` |
| Size | `w-n h-n size-n`, `w-full h-full`, `w-1/2` and other fractions, `min-w-n max-w-n min-h-n max-h-n` |
| Flex child | `grow` (weight 1), `grow-n`, `shrink-0` |
| Main axis (Row, Column) | `justify-start justify-center justify-end justify-between justify-around justify-evenly` |
| Cross axis (Row, Column) | `items-start items-center items-end items-stretch` |
| Child in a Box | `self-start self-center self-end`, `place-center`, `place-top-left` and the other eight positions |
| Background | `bg-{colour}` |
| Border | `border border-2 border-{colour}`, per side `border-t border-r border-b border-l` |
| Corners | `rounded rounded-{radius}` |
| Text | `text-{size}`, `text-{colour}`, `font-normal font-medium font-semibold font-bold`, `font-mono`, `italic underline line-through`, `leading-tight leading-normal leading-relaxed`, `tracking-tight tracking-wide`, `text-left text-center text-right`, `truncate`, `line-clamp-n` |
| Effects | `opacity-n`, `shadow-{size}`, `clip`, `blur-n`, `backdrop-blur-n` |
| Transform | `translate-x-n translate-y-n scale-n rotate-n` |
| Motion | `transition`, `duration-fast duration-normal duration-slow duration-[ms]`, `ease-standard ease-spring` |
| Interaction | `clickable focusable cursor-pointer cursor-text` |

## Variants

| Variant | Applies when |
|---|---|
| `hover:` | The pointer is over the node |
| `pressed:` | The pointer is down on it |
| `focus:` | It has keyboard focus |
| `disabled:` `selected:` `checked:` | The component reports that state |
| `dark:` `light:` | The active theme mode |
| `sm:` `md:` `lg:` `xl:` | The **window** is at least 640, 768, 1024, 1280 logical pixels wide |
| `compact:` | The theme's density is compact |

Variants stack: `dark:hover:bg-surface-raised`.

With `transition` on the node, a change caused by a variant animates over the theme's normal
duration instead of switching.

## What differs from Tailwind

- **No margins.** Space comes from the parent's `gap` and padding, as in Compose.
- **No `flex`, `flex-row`, `grid`, `block`, `hidden`.** The component decides the layout:
  `Row`, `Column`, `Box`, `Grid`. Classes style it.
- **Breakpoints measure the window**, not a screen or a container.
- **Colours are theme roles**, not a numbered palette. `bg-blue-500` does not exist;
  `bg-primary` or `bg-[#3B82F6]` does.
- **Order decides conflicts.** There is no specificity.
- **Nothing is inherited.** A text colour on a `Column` does not reach the `Text` inside it.

## Reuse

A class string is an ordinary `String`. Share one through a global literal or a function:

```prismio
let CARD = "p-4 bg-surface rounded-lg border"

fn dangerButton() -> String { return "bg-danger text-on-primary hover:opacity-90" }
```

Component defaults come from the theme; a component applies its defaults first and the
caller's classes after, so the caller wins.

## With `Modifier`

`cls("p-4 rounded-lg")` gives a `Modifier` to continue with computed values:
`cls("p-4 rounded-lg").width(panelWidth)`. Every component that takes a class string has an
overload taking a `Modifier` in the same position.

## Errors

An unknown class, an unknown variant, a value outside its scale, or a class that does not
apply to the component (`justify-between` on a `Text`) is reported with the class, the
component and the source of the string when known. Debug builds report once per distinct
string at run time; `xefy check` reports from source without running the app. Release builds
ignore the class.
