# 0010. Units, coordinates and colour

Status: accepted (2026-11-12)

## Decision
- All framework geometry is `F32` in **logical pixels** (Compose's dp). The engine multiplies
  by the window's scale factor; the framework never sees device pixels except to snap hairlines.
- Origin is top-left, x right, y down. Angles are radians, clockwise positive.
- Font sizes are logical pixels too; there is no separate sp unit in v0. A user text-scale
  setting is applied by the theme.
- Colour is a `U32` in ARGB order, `0xAARRGGBB`, non-premultiplied sRGB. It crosses the ABI as
  `uint32_t`.
- Time is `I64` nanoseconds from a monotonic clock. Durations in public APIs are milliseconds
  as `Int`.
- Text positions are UTF-8 byte offsets into the `String` (0011).
- `Int` is 32-bit in Prismio. Ids and timestamps use `I64` or `U64` explicitly.

## Consequences
Without operator overloading, geometry types get named functions (`offsetAdd`, `rectInset`).
Keep them in `xefy.geometry` so they can become operators in one place later.
