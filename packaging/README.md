# packaging

Per-platform packaging skeletons:

- `linux/` - desktop entry, AppImage / Flatpak recipes.
- `windows/` - NSIS / Inno installer.
- `macos/` - Info.plist template, .icns, and the build/sign/notarize
  runbook (`macos/README.md`) for the .app bundle + DragNDrop .dmg.
- `android/` - manifest + Gradle bits.
- `ios/` - Info.plist + bundle config.

`UPDATES.json` is the feed consumed by the in-app updater (desktop).
