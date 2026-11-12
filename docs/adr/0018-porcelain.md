# 0018. The visual direction is Porcelain

Status: accepted (2026-11-12)

## Context
Four directions were proposed under decision 0016: Graphite (dark, dense), Porcelain (light,
calm), Aurora (translucent, very round) and Signal (high contrast, sharp).

## Decision
- **Porcelain**: light first, warm neutral greys, white surfaces separated by hairline borders
  and soft shadows, 10-pixel corners on controls, one blue accent. A dark mode with the same
  structure ships alongside it.
- **Font:** Geist for UI text and Geist Mono for code, both under the SIL Open Font License.
- **Icons:** Phosphor, regular weight, under the MIT licence.
- Every value is a token in `docs/THEME.md`. That file is the source for `xefy.theme`.

## Consequences
- No backdrop blur or translucency is needed, so the look costs nothing extra on the DRM
  compositor.
- Geist covers Latin, Cyrillic and a few other scripts only. Text in any other script
  (Devanagari, Arabic, CJK) falls back to a second shipped family, which is still to be chosen;
  Noto Sans is the default candidate.
- The accent on white meets the 4.5:1 contrast minimum with no margin. A lighter accent would
  fail; changing it means re-checking the pairs listed in `docs/THEME.md`.
