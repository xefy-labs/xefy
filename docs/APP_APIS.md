# App-level APIs (v0)

What an app uses beyond composables: its own state, navigation, assets, strings, shortcuts and
overlays. Everything is in the v0 form (decisions 0008 and 0019). Signatures are the intended
shape; probe before relying on a construct.

## App state

```prismio
struct AppState { projects: Vec<Project>, selected: Int }

fn App(scope: Scope, state: AppState) {
    Row(scope, state, "h-full", |scope: Scope, state: AppState| {
        Sidebar(scope, state)
        Detail(scope, state)
    })
}

fn main() -> Int {
    let projects: Vec<Project> = []
    let state = AppState { projects: projects, selected: -1 }
    return runApp(appConfig("Projects", 1100, 720), state, |scope: Scope, state: AppState| { App(scope, state) })
}
```

- App data is one struct owned by `main`, passed beside `scope` (decision 0019).
- A container's four-parameter form passes it into the content closure. Never capture it.
- `remember*` is for view state only. After changing app data, call `invalidate(scope)`.

## Navigation (`xefy.nav`)

A navigator is a stack of route ids kept in `Scope` state. A route id is an `Int` constant the app
defines; route arguments live in app state.

| Function | Does |
|---|---|
| `rememberNav(scope, startRoute) -> Int` | Creates or finds the navigator; returns its handle |
| `navCurrent(scope, nav) -> Int` | The route on top |
| `navPush(scope, nav, route)` | Go forward |
| `navPop(scope, nav) -> Bool` | Go back; false at the root |
| `navReplace(scope, nav, route)` | Swap the top |
| `navCanPop(scope, nav) -> Bool` | |

The app chooses what to show with an `if` chain or `match` on `navCurrent`. On desktop the
usual shape is a `Sidebar` that calls `navReplace` and a content area that reads `navCurrent`.
Mouse back and forward buttons and the platform's back shortcut call `navPop` on the innermost
navigator under the pointer. Transitions between routes are tier 2.

## Assets (`xefy.assets`)

- An asset is named by its path as written in `config.xf`: `Image(scope, "assets/images/logo.png")`.
- The loader looks in the app bundle's resource directory, or in the project directory when
  run with `xefy run`. The engine reports the resource directory (an addition to the ABI:
  `xefy_app_get_resource_path`).
- Decoded images are cached by path. A missing asset draws a visible placeholder and logs once.
- `@2x` and `@3x` variants beside a file are chosen by the window's scale factor.
- Fonts declared in `config.xf` are registered at start-up; the framework's own fonts and
  icons are registered first.

## Strings (`xefy.i18n`)

- `tr(scope, "greeting")` returns the string for the current locale from `i18n/<locale>.xf`.
- `trWith(scope, "welcome", values)` replaces `{0}`, `{1}` with the given strings.
- The locale comes from the system (an addition to the ABI: `xefy_app_get_locale`), then the
  language without its region, then `en`. A missing key returns the key itself and logs once.
- Plural forms and number and date formatting are tier 2.

## Keyboard shortcuts

`shortcut(scope, "mod+s")` returns true on the frame the chord was pressed, unless a focused text
field consumed the key. `mod` is Command on macOS and Control elsewhere. Chords are written
lowercase with `+`: `mod+shift+p`, `alt+left`, `escape`, `f5`. A `Menu` item takes the same
string and shows it formatted for the platform.

## Overlays: dialogs, popovers, menus, tooltips

- The frame has two roots: the main tree and an overlay layer painted after it and hit-tested
  before it.
- An overlay is opened by a `Bool` state handle: `Dialog(scope, open, "w-[420]", |scope: Scope| { ... })`
  composes its content into the overlay layer only while `open` is true.
- `Dialog` dims the main tree, traps focus, and closes on Escape or a click outside (it sets
  the handle to false). It restores focus to what had it before.
- `Popover`, `Menu` and `Tooltip` are anchored to a node: they take the anchor's id and place
  themselves from its rectangle of the last frame, flipping to stay inside the window.
- Only the topmost overlay receives keyboard input.

## Focus and accessibility groundwork

- Every interactive composable is reachable with Tab in tree order and shows the focus ring.
- A composable declares a role and a label when it creates its node (`button`, `checkbox`,
  `textfield`, `image`, ...). v0 stores them and does nothing else; the accessibility bridge
  reads them later without changing composable code.

## Windows

v0 has one window per app, created by `runApp` from `appConfig` and the `window` block of
`config.xf`. `setWindowTitle(scope, title)` changes its title. More windows are tier 2.
