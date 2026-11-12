# JetBrains plugin

Design of the Xefy plugin for IntelliJ-based IDEs (decision 0020). The plugin is not written
yet and lives in its own repository; this file specifies what it does and the contract the
`xefy` tool provides for it.

Plugin id `io.xefy`, depends on `io.prismio`, Kotlin, IntelliJ Platform 2026.2 or later, every
JetBrains IDE. Apache 2.0.

## Principles

1. Prismio language support (highlighting, completion, errors, navigation, formatting,
   debugging) belongs to the Prismio plugin. The Xefy plugin adds to it and never replaces it.
2. No Xefy knowledge is compiled into the plugin. It asks the project's `xefy` tool.
3. Every feature degrades to "off, with a one-line reason" when the tool is missing or too old.

## Features

### Phase 1: needs only the CLI

| Feature | What the user gets |
|---|---|
| **Project detection** | A directory with `config.xf` is a Xefy app. The SDK is found from the `XEFY_HOME` environment variable or a setting under Languages & Frameworks, Xefy; its version is shown |
| **New Project, Xefy** | Name, identifier, location; runs `xefy create`; opens `src/main.psm` |
| **`config.xf` and `i18n/*.xf`** | Highlighting, brace matching, folding, structure view, formatting. Completion of blocks, settings and allowed values with documentation, from the schema. Errors as you type from `xefy check` |
| **Class strings** | Inside a string literal that the tool reports as a class string: completion of utilities, variants and token values; hover showing what a class resolves to (`p-4`: padding 16); a colour swatch in the gutter for colour classes, for the light and dark theme; unknown or misapplied classes underlined with a quick fix to the nearest valid name; an intention to sort classes into the canonical order |
| **Colour literals** | A swatch in the gutter for `0xAARRGGBB` literals, with the colour picker |
| **Run** | Run configurations for `xefy run`, `xefy build` and `xefy bundle`, one per target in `config.xf`; a run button in the gutter beside `fn main`; compiler errors in the output link to the line |
| **Asset and string keys** | Completion and go-to-file for asset paths and `tr` keys; a warning for a path or key that does not exist |
| **Templates** | Live templates (`col`, `row`, `box`, `btn`, `txt`, `comp` for a composable function); New, Xefy Composable and New, Xefy Screen file templates |
| **Doctor** | An action that runs `xefy doctor` and shows the result |

### Phase 2: needs the engine's offscreen surface and the runtime

| Feature | What the user gets |
|---|---|
| **Composable preview** | A tool window showing a composable rendered by `xefy preview`, refreshed on save; light and dark, and a choice of window widths for breakpoint variants. A composable opts in with a zero-argument preview function |
| **Gallery** | Browse the SDK's composables with their class defaults, and insert one |

### Phase 3: needs Prismio features

| Feature | Depends on |
|---|---|
| **Hot reload** from the Run tool window, keeping app state | Prismio X36 |
| **Layout inspector**: pick a node in the running app, see its rectangle, classes and state | A debug channel in the runtime |

## The contract with the `xefy` tool

All commands print one JSON object on standard output, exit 0 when they could answer, and
include `"protocol": 1`. The plugin checks `protocol` and refuses a higher major number.

| Command | Answers |
|---|---|
| `xefy ide info` | SDK version, protocol version, SDK path, which features this SDK supports |
| `xefy ide schema` | The `config.xf` schema: every block, setting, call, value type, allowed values, default and one-line documentation |
| `xefy ide vocabulary` | Every utility with its value kind and what it sets; every variant; every scale; every theme token with its light and dark value; which utilities apply to which composable |
| `xefy check --format json [--stdin <path>]` | Diagnostics for `config.xf`, `.xf` string files, class strings, asset paths and `tr` keys. Each has a file, a range (line and column, start and end), a severity, a stable code, a message and optional replacement text. Each class-string literal found in `.psm` files is listed with its range and the composable it is passed to, so the editor knows where to offer completion. `--stdin` checks unsaved editor content for one file |
| `xefy ide assets` | Asset keys and string keys with the file and line that declare them |
| `xefy preview <file> <function> --theme <mode> --width <n> --out <png>` | Renders one preview function offscreen (phase 2) |

The plugin caches `schema` and `vocabulary` per SDK version and reruns `check` on a debounce
after edits.

## How class strings are found

The plugin does not guess. `xefy check` reports each class-string literal: a string literal
passed in the style position of a known composable, or assigned to a global that is later
passed in one. Between runs of `check`, the plugin keeps treating a literal as a class string
if its range overlaps one reported by the last run.

## Repository layout

```
jetbrains-plugin/
  build.gradle.kts  settings.gradle.kts  gradle.properties
  src/main/kotlin/io/xefy/
    sdk/         locating the SDK, running the tool, parsing and caching its JSON
    config/      config.xf file type, lexer, parser, completion, annotator, structure view
    classes/     class-string injection, completion, hover, swatches, inspection, sorter
    run/         run configurations, gutter markers, console filters
    project/     New Project wizard, file templates, live templates
    preview/     the preview tool window (phase 2)
  src/main/resources/META-INF/plugin.xml
  src/test/      tests against recorded JSON from the tool
```

`config.xf` uses the block grammar of `build.ums`, which the Prismio plugin already parses. If
that plugin exposes its UMS lexer and parser, the Xefy plugin reuses them; otherwise it carries
its own small parser for the same grammar.

## Testing

Tests run against JSON recorded from a real `xefy` tool and stored in the plugin repository,
so they need no SDK. One integration test runs against an installed SDK when `XEFY_HOME` is set.
