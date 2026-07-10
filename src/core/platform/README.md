# core/platform

OS integration behind interfaces, so the rest of the app stays `#ifdef`-free.
Target: `da_platform` -> `da_domain`.

- `iplatform_services.h` - the interface (tray + tray-watch, notifications,
  updates).
- `desktop/` - desktop implementations (link Qt Widgets + vendored desktop deps).
- `mobile/` - no-op stubs for Android/iOS, also used by the browser (wasm) build.
- `single_instance.h` - the GUI's per-user single-instance guard
  (raise-existing-window protocol over QLocalServer).
- `autostart/` - launch-at-login seam (`da_autostart`, QtCore-only so the TUI
  links it without QtWidgets): pure command/desktop-entry/registry logic, one
  thin backend per OS (HKCU Run key / SMAppService / XDG autostart), and the
  shared `AutostartController` both settings surfaces bind.
