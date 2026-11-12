# 0007. Reusing Flutter: port Dart, copy C++, record provenance

Status: accepted (2026-11-12)

## Context
Flutter's framework is Dart and its engine is C++. Xefy must not depend on Flutter or Dart at
build or run time, and Flutter's BSD-3 licence requires attribution to be kept.

## Decision
- **Port** (Dart to Prismio): the file's header comment names the upstream path and the
  revision pinned in `deps.lock.json`, and says what was changed.
- **Copy** (C++ kept as C++): into `vendor/flutter/` only, with an entry in
  `vendor/flutter/PROVENANCE.json` (`path`, `upstream_path`, `revision`, `changes`).
  `python3 xefy.py provenance` fails when an entry is missing.
- `third_party/flutter` is read-only reference. Nothing includes, imports or links it.
- Public names are Xefy's and follow Compose, not Flutter (`Column`, not `RenderFlex`), though
  internal names may stay close to upstream to keep ports reviewable.

## Consequences
A port is a translation task with a clear oracle: the upstream file and its tests. That makes
ports good work for smaller models, one file or one algorithm per task.
