# 0024. The OS is named AOS

Status: accepted (2026-11-12). Changes the name fixed in 0021.

## Decision
The desktop OS that Xefy targets is **AOS**, not XefyOS. It has its own repository; `os/` in
this repository stays reserved for the Xefy side of it. The `config.xf` platform block is
`aos { }`. `.xapp` keeps its name.

## Consequences
Applied to every document, the app template and earlier records on the same day, as 0021 was.
Record 0015 is renamed to `0015-aos-app-model.md`.
