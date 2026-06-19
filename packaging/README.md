# packaging

Per-platform packaging skeletons:

- `linux/` - desktop entry, AppImage / Flatpak recipes.
- `windows/` - NSIS / Inno installer.
- `macos/` - bundle + dmg.
- `android/` - manifest + Gradle bits.
- `ios/` - Info.plist + bundle config.

`UPDATES.json` is the feed consumed by the in-app updater (desktop).
