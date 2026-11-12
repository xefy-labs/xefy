# 0009. Nodes live in an arena and are addressed by index

Status: accepted (2026-11-12)

## Context
A UI tree holds nodes of many kinds that point at parents and children. Prismio has no stored
trait objects, no shared ownership and no weak references.

## Decision
- One `Vec` of node records per frame (the arena). A node is referred to by its `Int` index;
  parent, first child, last child and next sibling are indices, with -1 for none.
- A node record holds what every kind shares (kind tag, links, id, constraints, size, offset,
  modifier range). Data specific to a kind lives in a per-kind `Vec` and the record holds an
  index into it.
- Per-kind behaviour is a `match` on the kind tag inside `layout`, `paint` and `hitTest`.
- A node also has a stable 64-bit **id**: a hash of its parent's id, its kind, and its position
  among its siblings or an explicit key. Everything that must survive a frame (state slots,
  focus, hover, scroll offset, last frame's rectangle) is keyed by id in maps, not stored in
  the node.

## Consequences
No ownership cycles and nothing to free by hand: the arena is cleared at the start of a frame.
Third-party node kinds are not possible in v0; a custom-drawn component uses the `Canvas` node.
Adding a node kind means touching the kind tag and each `match`, so each pass keeps its
`match` in one function.
