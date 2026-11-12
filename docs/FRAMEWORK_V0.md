# Framework v0 design

The design of `framework/xefy/` as it can be built on Prismio 0.2.0 today. Decisions and their
reasons are in `docs/adr/`; this file is the detail an implementer needs. Read 0003, 0004, 0008,
0009 and 0010 first.

Signatures below are the intended shape, not checked code. Prismio is young: before writing a
module, probe each construct you rely on with a five-line file and `prismio check`. When a
construct is refused, use the nearest working form, keep the public names in this file, and say
so in your report. The Prismio repository's `docs/XEFY_PLAN.md` lists what is known not to work.

## What an app looks like

```prismio
import xefy.app
import xefy.runtime
import xefy.ui
import xefy.modifier

fn Counter(scope: Scope) {
    let count = rememberInt(scope, 0)
    Column(scope, "p-6 gap-3", |scope: Scope| {
        Text(scope, "Clicked " + getInt(scope, count).toString() + " times")
        if Button(scope, "Add", "hover:bg-primary-hover transition") {
            setInt(scope, count, getInt(scope, count) + 1)
        }
    })
}

fn main() -> Int {
    return runApp(appConfig("Counter", 480, 320), |scope: Scope| { Counter(scope) })
}
```

Rules every composable follows:

- `scope: Scope` is the first parameter. A container's content closure takes `scope: Scope` as its only
  parameter and the container passes its own `scope` in. `scope` is never captured.
- A composable that can be activated returns `Bool`: true on the frame after the activation.
- A container also has a four-parameter form, `(scope, state, classes, content)`, that passes the
  app's own state into a content closure of the form `|scope: Scope, state: T|` (decision 0019,
  `docs/APP_APIS.md`).
- Optional settings come through overloads by parameter count. The order is always
  `(scope, required..., style, content)`, where `style` is a class string or a `Modifier`
  (decision 0017). Each composable has both overloads and one without `style`.
- Public composable names are capitalised (`Column`, `Text`); everything else is camelCase.
- A composable never keeps a `String`, a `Vec` or a struct in a local that a later content
  closure also needs: the closure would move it. Pass such values as arguments, or keep them
  in `Scope` state and capture the `Int` handle.

## Modules

One file per module under `framework/xefy/`. Imports are not transitive, so each file imports
exactly what it uses. A module may import only modules above it in this table.

| Module | Holds | Imports |
|---|---|---|
| `xefy.ffi` | Generated `extern fn` declarations and `repr(C)` structs for `xefy_engine.h`. Never edited by hand | none |
| `xefy.geometry` | `Offset`, `Size`, `Rect`, `EdgeInsets`, `Constraints` and functions on them | none |
| `xefy.color` | ARGB `U32` helpers: `rgb`, `argb`, `withAlpha`, `lerpColor`, named constants | none |
| `xefy.gfx` | `Canvas`, `Paint`, `Path`, `Image`, `Paragraph`, `FontCollection`: thin wrappers that own an engine handle | ffi, geometry, color |
| `xefy.platform` | `App`, `Window`, `Event`, clipboard, cursor, IME: wrappers over the engine's platform half | ffi, geometry |
| `xefy.animation` | Curves (the cubic solver is already ported), tweens, spring simulation | none |
| `xefy.modifier` | `Modifier` and its operations | geometry, color |
| `xefy.theme` | `Theme`: colours, type scale, spacing, shape radii | color |
| `xefy.tint` | Class strings: parse, cache, and resolve to `ModifierOp`s for a node's state (`docs/TINT.md`) | modifier, theme, color |
| `xefy.tree` | The node arena, node kinds, per-kind data, stable ids | geometry, color, modifier |
| `xefy.runtime` | `Scope`: tree, state pools, input snapshot, interaction state, theme, frame clock | tree, geometry, theme, platform, gfx |
| `xefy.layout` | The layout pass | tree, geometry, gfx |
| `xefy.paint` | The paint pass | tree, geometry, gfx, color |
| `xefy.hit` | Hit-testing and the interaction rules (hover, press, focus, scroll target) | tree, geometry, runtime |
| `xefy.ui` | Every public composable, `remember*`, `key` | runtime, tree, modifier, theme, geometry |
| `xefy.app` | `appConfig`, `runApp`: the event loop | everything |

## Geometry (`xefy.geometry`)

```prismio
public struct Offset { public x: F32, public y: F32 }
public struct Size { public width: F32, public height: F32 }
public struct Rect { public left: F32, public top: F32, public right: F32, public bottom: F32 }
public struct EdgeInsets { public left: F32, public top: F32, public right: F32, public bottom: F32 }
public struct Constraints {
    public minWidth: F32, public maxWidth: F32,
    public minHeight: F32, public maxHeight: F32
}
```

`Constraints` is Flutter's `BoxConstraints` (`rendering/box.dart`): port `tight`, `loose`,
`constrain`, `deflate`, `enforce`, `hasBoundedWidth`, `isTight` and their exact semantics.
Infinity is `F32` infinity. Functions are free functions named `<type><Verb>`:
`rectContains(r, p)`, `rectInset(r, insets)`, `sizeConstrain(c, s)`.

Structs are move-only in Prismio. Geometry values are small, so functions return new values
and callers rebuild rather than share.

## The node tree (`xefy.tree`)

```prismio
public struct Node {
    public kind: Int,          // NODE_* constant
    public id: U64,            // stable across frames
    public parent: Int,        // arena index, -1 for the root
    public firstChild: Int, public lastChild: Int, public nextSibling: Int,
    public data: Int,          // index into the Vec for this kind
    public modStart: Int, public modCount: Int,   // range in Tree.mods
    // written by layout:
    public x: F32, public y: F32,                 // offset in the parent
    public width: F32, public height: F32
}

public struct Tree {
    public nodes: Vec<Node>,
    public mods: Vec<ModifierOp>,
    public flex: Vec<FlexData>, public text: Vec<TextData>, public image: Vec<ImageData>,
    public scroll: Vec<ScrollData>, public canvas: Vec<CanvasData>
}
```

Node kinds in v0: `NODE_BOX` (stacks children, like Compose's `Box`), `NODE_FLEX` (row or
column), `NODE_TEXT`, `NODE_IMAGE`, `NODE_SPACER`, `NODE_SCROLL`, `NODE_CANVAS`. Every other
composable is built from these plus modifiers.

**Ids.** `id = hash64(parent.id, kind, ordinal)`, where `ordinal` is the node's position among
its siblings, or the explicit key from `key(scope, k)`. The hash is FNV-1a over those three
values. The root's id is a constant. Two siblings with the same id is a bug: report it once per
frame in debug builds.

**Lifetime.** `treeReset` clears every `Vec` at the start of a frame. Nothing outside the
frame holds a node index; it holds an id.

## Modifiers (`xefy.modifier`)

```prismio
public enum ModifierOp {
    Padding { left: F32, top: F32, right: F32, bottom: F32 },
    Size { width: F32, height: F32 },          // negative = unconstrained on that axis
    FillMaxWidth { fraction: F32 }, FillMaxHeight { fraction: F32 },
    Weight { value: F32 },                     // flex factor inside Row / Column
    Align { x: F32, y: F32 },                  // -1..1, inside a Box
    Background { color: U32, radius: F32 },
    Border { color: U32, width: F32, radius: F32 },
    Clip { radius: F32 },
    Opacity { value: F32 },
    Offset { x: F32, y: F32 },
    Spacing { value: F32 },                    // gap between children of a Row / Column
    Clickable, Focusable
}

public struct Modifier { public ops: Vec<ModifierOp> }
public fn modifier() -> Modifier
impl Modifier { fn padding(self, all: F32) -> Modifier ... }   // one method per op, each returns self
```

Order matters and follows Compose: operations apply outside-in in the order written, so
`modifier().padding(8.0).background(c)` pads outside the background. Layout walks a node's
ops in order, wrapping constraints; paint walks them in the same order, drawing and
translating.

## `Scope` (`xefy.runtime`)

`Scope` is the one mutable value. It holds:

- **`tree`**: this frame's `Tree`, and `current`: the arena index children attach to.
- **State pools**: `ints: Vec<Int>`, `floats: Vec<F32>`, `bools: Vec<Bool>`,
  `strings: Vec<String>`, and a map from slot key to pool index. A slot key is
  `hash64(ownerId, slotOrdinal)` where `slotOrdinal` counts `remember*` calls made while that
  owner is current. `rememberInt(scope, init)` returns the existing handle or creates one.
  Slots whose owner did not appear for a frame are released at the end of that frame.
- **Retained per-id data**: last frame's rectangle in window coordinates, scroll offsets,
  running animations, cached paragraphs. Maps from id.
- **Input snapshot** for this frame: pointer position, buttons down, pressed and released
  edges, scroll delta, key events, committed text, modifiers.
- **Interaction state**: `hoverId`, `activeId` (pointer is down on it), `focusId`.
- **`theme`**, the frame time, the window size and scale, and `needsFrame`.

`setInt` and the other setters set `needsFrame`, so a change is drawn on the next frame.

## One frame (`xefy.app`)

`runApp(config, root)` creates the app and window, then loops:

1. `poll` with a timeout of -1 when idle, 0 while `needsFrame` is set. Fold each event into the
   input snapshot. Pointer, key, text, resize and scale events set `needsFrame`.
2. When `needsFrame` is set and no frame is pending, `requestFrame`.
3. On a `FRAME` event:
   1. `scopeBegin`: reset the tree, resolve interaction from the snapshot and **last frame's**
      rectangles (`xefy.hit`).
   2. Call `root(scope)`. Composables append nodes and read interaction state.
   3. `layout(tree, root, tight(windowSize))`.
   4. Record every node's window rectangle by id for the next frame.
   5. `beginFrame`, `paint(tree, canvas)`, `endFrame`.
   6. `scopeEnd`: clear input edges, release dead slots. If anything is animating or a setter
      ran, leave `needsFrame` set.
4. On `QUIT_REQUESTED` or the last window closing, leave the loop and return 0.

## Layout (`xefy.layout`)

`layoutNode(tree, index, constraints) -> Size` sizes one node and positions its children, by
Flutter's protocol. A node is laid out once per frame; there are no relayout boundaries or
dirty flags in v0, because the tree is rebuilt anyway.

- **Modifiers first.** Walk the ops in order: `Padding` deflates the constraints and offsets
  the content; `Size`, `FillMax*` tighten them. Then lay out the kind, then walk back out
  adding padding to the size.
- **`NODE_FLEX`**: port `RenderFlex.performLayout` from
  `third_party/flutter/packages/flutter/lib/src/rendering/flex.dart`: inflexible children
  first with unbounded main axis, then free space divided by `Weight`, then main-axis
  alignment and cross-axis alignment. Keep its handling of unbounded constraints. Baseline
  alignment and text direction are out of scope for v0.
- **`NODE_BOX`**: port `RenderStack` (`rendering/stack.dart`) without positioned children:
  size to the largest child unless constrained, place each by its `Align` op.
- **`NODE_TEXT`**: build or reuse the paragraph, `layout(maxWidth)`, size to its longest line
  and height, constrained.
- **`NODE_SCROLL`**: lay the child out unbounded on the scroll axis, size self to the
  constraints, clamp the stored offset.
- **`NODE_IMAGE`**, **`NODE_SPACER`**, **`NODE_CANVAS`**: size from data and constraints.

Layout must be testable with no window: text measurement goes through one function that tests
can replace with a fixed-advance measurer.

## Paint (`xefy.paint`)

`paintNode(tree, index, canvas)`: save, translate by the node's offset, walk modifier ops
(`Opacity` and `Clip` open a layer or clip, `Background` and `Border` draw, `Padding` and
`Offset` translate), draw the kind, paint children in order, restore. Later siblings paint on
top.

## Input and interaction (`xefy.hit`)

Resolved once per frame, before composition, against last frame's rectangles:

- **Hover**: the topmost id with `Clickable` whose rectangle contains the pointer.
- **Press**: on pointer down, `activeId = hoverId`. On pointer up, if `activeId == hoverId`
  that id is *activated* this frame; then `activeId` clears. Moving off and releasing cancels.
- **Focus**: pointer down on a `Focusable` id focuses it; Tab and Shift+Tab move focus in tree
  order; pointer down elsewhere clears it.
- **Keyboard activation**: Space or Enter on the focused clickable id activates it.
- **Scroll**: wheel deltas go to the innermost `NODE_SCROLL` under the pointer that can still
  scroll in that direction.

A composable asks by its own id: `isHovered(scope, id)`, `isPressed`, `wasActivated`,
`hasFocus`. Because a composable knows its id as soon as it creates its node, `Button` is: make
the node, read `wasActivated`, pick colours from hovered and pressed, add the label, return
the flag.

## Testing

- Pure modules (`geometry`, `color`, `animation`, `modifier`, `tree`, `layout`) are tested with
  `prismio test` and no engine. Layout tests build a tree by hand, run layout with the fixed
  measurer and assert rectangles. Port Flutter's cases from `packages/flutter/test/rendering/`
  where a port exists.
- Composable tests run a root function against a `Scope` with a scripted input snapshot and
  assert on the tree and on state, for two or three frames. No window.
- Drawing is tested by rendering to an offscreen raster surface and comparing against a
  stored PNG, once the engine has an offscreen surface.

## What v0 leaves out

Accessibility semantics, right-to-left layout, baseline alignment, lazy lists (a `Column` in a
scroll node composes every child), positioned children in a `Box`, shared-element transitions,
multiple windows, IME composition, drag and drop, and gestures beyond press, hover and scroll.
