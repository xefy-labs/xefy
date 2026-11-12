# 0015. AOS app model

Status: accepted (2026-11-12). Details open until the Linux embedders exist.

## Decision
- The kernel is Linux. An app is an ordinary dynamically or statically linked ELF executable;
  nothing about an app is freestanding.
- The system compositor and shell are a Xefy app on the `linux_drm` embedder (DRM/KMS, GBM and
  EGL or Vulkan, libinput).
- An app ships as a `.xapp` bundle: executable, assets, a manifest generated from `config.xf`,
  and a signature. `permissions` in the manifest are enforced by the OS.
- Until Prismio has a stable ABI, each app links the framework statically.

## Open
How an app's surface reaches the compositor: the Wayland protocol, or a private protocol over a
Unix socket with dmabuf. Wayland lets existing Linux apps and drivers work; a private protocol
is smaller and fully owned. To be decided before M5.
