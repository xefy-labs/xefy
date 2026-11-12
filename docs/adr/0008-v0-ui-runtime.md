# 0008. The v0 UI runtime: explicit `Scope` context, immediate-mode events

Status: accepted (2026-11-12). Replaced when the Prismio features in 0006 land.

## Context
Probed on Prismio 0.2.0: globals hold only literals; closures cannot be stored; a closure
captures by value, so a struct it captures is moved and a mutation of a captured local is lost;
`dyn Trait` cannot be stored; there are no default or named arguments. What does work: a
generic closure parameter called within the call, nested block closures, overloads by parameter
count, a struct borrowed through an ordinary parameter with assignable fields, scalars copied
into closures.

## Decision
```prismio
fn App(scope: Scope) {
    let count = rememberInt(scope, 0)
    Column(scope, |scope: Scope| {
        Text(scope, "Clicked " + getInt(scope, count).toString() + " times")
        if Button(scope, "Add") { setInt(scope, count, getInt(scope, count) + 1) }
    })
}
```
- **Context.** `main` owns one `Scope`. Every composable takes it first; a container hands it back
  to its content closure as that closure's parameter. It is never captured.
- **Events.** A control returns whether it was activated since the previous frame. No handler
  is stored.
- **State.** Values live in typed pools inside `Scope`; a handle is a bare `Int`. A slot is
  identified by its owner's id and its call order, with `key(scope, k)` for loops.
- **Options.** Overloads by parameter count, plus a `Modifier` value built by chained methods.
- **Recomposition.** The root function runs in full on every frame that is drawn; the node
  tree is rebuilt each time. Identity, state and interaction survive through stable node ids.

## Consequences
- This is the API the framework and its tests are built on now. It is not the API Xefy ships.
- Composable names, the `Modifier` vocabulary and the layout model are the final ones, so moving
  to the target API is mechanical: drop `scope`, turn `if Button(...) { }` into `onClick`, turn
  `getInt`/`setInt` into `count.value`.
- A control's hit area is last frame's layout, so input is resolved one frame late, and a state
  change requests one more frame. Both are normal for immediate-mode UI.
- Full details: `docs/FRAMEWORK_V0.md`.
