# core/platform

OS integration behind interfaces, so the rest of the app stays `#ifdef`-free.
Target: `da_platform` -> `da_domain`.

- `iplatform_services.h` - the interface (tray, autostart, updates, global
  shortcut, single-instance).
- `desktop/` - desktop implementations (link Qt Widgets + vendored desktop deps).
- `mobile/` - no-op stubs for Android/iOS, also used by the browser (wasm) build.
