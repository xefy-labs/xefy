# 0023. The OS shell runs on an existing Wayland compositor

Status: proposed (2026-11-12). Changes 0015. Not applied to other documents until accepted.

## Context
0015 made the system compositor a Xefy app on a `linux_drm` embedder and left open whether an
app's surface reaches it over Wayland or a private protocol. The OS project (AOS) has since
set the requirement that nothing is written that a maintained open-source project already
provides. Flatpak apps, GPU drivers and every existing Linux app speak Wayland or X11 through
XWayland. A compositor with
HDR, variable refresh, direct scanout, fractional scaling and XWayland is years of work.

## Decision
- The display protocol is Wayland. The private protocol option in 0015 is closed.
- The compositor is an existing one (KWin; a wlroots compositor is the fallback). Xefy does
  not implement a compositor.
- The shell (bar, dock, launcher, notifications, control centre) is a set of Xefy apps that
  are Wayland clients on the `linux_wayland` embedder, using layer-shell surfaces.
- The `linux_drm` embedder is no longer on the OS path. It stays possible for kiosk and
  embedded use and is not scheduled.
- An app for the OS ships as a Flatpak built from its `.xapp` contents. `permissions` in
  `config.xf` map to Flatpak permissions and portals, not to a new sandbox.

## Consequences
- The `linux_wayland` embedder becomes the most important embedder after macOS, and needs
  more than a plain window: see `docs/AOS_PLAN.md`.
- The ABI needs a way to wait on foreign file descriptors in the pull loop (0004), because a
  shell process talks to D-Bus while it waits for input.
- Milestone M6 in `ARCHITECTURE.md` ("AOS compositor and shell") becomes "AOS shell".
- If accepted, `ARCHITECTURE.md`, `os/README.md`, `engine/embedder/README.md` and 0015 are
  updated to match.
