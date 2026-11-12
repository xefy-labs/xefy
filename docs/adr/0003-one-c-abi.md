# 0003. One C ABI between framework and engine

Status: accepted (2026-11-12)

## Context
Prismio calls C through hand-declared `extern fn`. Its FFI has measured limits: structs cross
only by pointer, integers narrower than 32 bits arrive unextended on Apple arm64, there are no
callbacks that carry state and no unwinding.

## Decision
`engine/abi/xefy_engine.h` is the only native surface the framework sees. Its rules:
1. C99, `extern "C"`, includes only `<stdint.h>` and `<stddef.h>`.
2. No struct by value in either direction; `const T*` in, `T*` out.
3. No parameter or return narrower than 32 bits. Booleans are `int32_t`.
4. Geometry is `float`; time is `int64_t` nanoseconds.
5. Strings are pointer plus byte length, UTF-8, never NUL-dependent. Returned strings are
   copied into a caller buffer.
6. Objects are opaque handles with explicit create and destroy.
7. Every function returns an `int32_t` status; results come back through out-pointers.
8. Structs that may grow start with `uint32_t struct_size`.
9. Additive changes bump MINOR, breaking changes bump MAJOR.

## Consequences
The Prismio declarations are generated from the header by a Python script, never written by
hand. Every embedder and the Skia shim implement the same header, so the framework has no
platform-specific code. 3D gets its own optional header later.
