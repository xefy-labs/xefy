# config.xf

Every Xefy app has one `config.xf` at its root. It names the app, its window, assets,
dependencies, optional features, permissions and per-platform settings. The `xefy` tool reads it
and generates everything a platform needs (`.xefy/build.ums`, bundle metadata, the `.xapp`
manifest), so an app holds no link flags, `Info.plist` or desktop file of its own unless it
overrides one under `platform/`.

> Status: this is the format's specification. The parser in `cli/` is not written yet.

## Grammar

`config.xf` uses the block grammar of Prismio's `build.ums`:

```
file    = block*
block   = IDENT args? "{" entry* "}"
entry   = IDENT "=" value            // setting
        | IDENT args                 // call
        | block                      // nested block
args    = "(" value ("," value)* ")"
value   = STRING | INT | FLOAT | "true" | "false"
comment = "//" to end of line
```

Strings are double-quoted. A setting may appear once per block. An unknown block or setting is
an error, so a typo is reported rather than ignored.

## Blocks

### `app` (required)

| Setting | Type | Meaning |
|---|---|---|
| `name` | string | Package name: lowercase letters, digits and `_`, starting with a letter |
| `id` | string | Reverse-DNS identifier, unique per app: `com.example.notes` |
| `version` | string | `major.minor.patch` |
| `xefy` | string | Framework version the app is written against |
| `entry` | string | Entry source file. Default `src/main.psm` |
| `displayName` | string | Name shown to users. Default: `name` |

### `window`

The first window. Sizes are logical pixels.

| Setting | Type | Default |
|---|---|---|
| `title` | string | `app.displayName` |
| `width`, `height` | int | 960, 640 |
| `minWidth`, `minHeight` | int | none |
| `resizable` | bool | `true` |
| `decorations` | bool | `true` |

### `assets`

| Call | Meaning |
|---|---|
| `dir("assets/images")` | Bundle every file below the directory |
| `file("assets/logo.png")` | Bundle one file |
| `font("Inter") { file("assets/fonts/Inter-Regular.ttf") weight = 400 }` | Register a font family; repeat `file` blocks for more weights and styles |

Paths are relative to the app root and are the keys the app loads assets by.

### `dependencies`

| Call | Meaning |
|---|---|
| `package("xefy.charts") { version = "0.3" }` | A package from the registry |
| `path("../shared_ui")` | A package in a local directory |
| `git("https://example.com/ui.git") { rev = "..." }` | A package at a pinned revision |

Resolved versions are written to `xefy.lock`.

### `features`

Optional parts of the framework. A feature that is off is not linked.

| Setting | Default | Meaning |
|---|---|---|
| `three` | `false` | 3D scenes (`Scene3D`), which links bgfx |

### `permissions`

What the app may do. XefyOS enforces these from the installed manifest; other platforms map
them to their own mechanism where one exists (macOS entitlements) and otherwise ignore them.

| Setting | Values |
|---|---|
| `network` | `true`, `false` |
| `files` | `"none"`, `"user-selected"`, `"documents"`, `"all"` |
| `camera`, `microphone`, `location` | `true`, `false` |

### `targets`

One block per platform the app builds for. A platform with no block uses defaults.

| Block | Settings |
|---|---|
| `macos` | `minOs`, `category`, `signing { identity = "..." }` |
| `windows` | `minOs`, `icon` |
| `linux` | `backend` = `"wayland"` or `"x11"` |
| `xefyos` | `format` = `"xapp"` |

### `native`

C or C++ the app brings itself, passed through to `build.ums`.

| Call | Meaning |
|---|---|
| `source("native/audio.c")` | Compile and link a source file |
| `include("native/include")` | Add an include directory |
| `library("z")` | Link a system library |

## Example

```
app {
    name    = "notes"
    id      = "com.example.notes"
    version = "1.0.0"
    xefy    = "0.1"
}

window {
    title  = "Notes"
    width  = 960
    height = 640
}

assets {
    dir("assets/images")
    font("Inter") { file("assets/fonts/Inter-Regular.ttf") weight = 400 }
}

dependencies {
    path("../shared_ui")
}

permissions {
    files = "user-selected"
}

targets {
    macos  { minOs = "13.0" }
    linux  { backend = "wayland" }
    xefyos { format = "xapp" }
}
```

## Strings files

Files under `i18n/` use the same grammar with one block:

```
strings {
    greeting = "Hello"
}
```
