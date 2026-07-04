# packaging

Per-platform packaging skeletons:

- `linux/` - desktop entry, AppImage / Flatpak recipes.
- `windows/` - NSIS / Inno installer.
- `macos/` - bundle + dmg.
- `android/` - manifest + Gradle bits.
- `ios/` - Info.plist + bundle config.

`UPDATES.md` is the release-feed / auto-update design (capability dial, signed
`manifest.json` schema, threat model). `UPDATES.json` is only the legacy
QSimpleUpdater-format mirror of that feed (synced by
`scripts/sync-updates-json.sh`).
