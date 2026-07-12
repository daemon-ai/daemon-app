# packaging

Per-platform packaging skeletons:

- `icons/` - the two source SVGs (`small.svg`, `large.svg`) + the icon
  pipeline's regen/drift docs (`icons/README.md`). Every committed icon in the
  dirs below is generated from these two files.
- `linux/` - desktop entry, AppImage / Flatpak recipes, hicolor icon set.
- `windows/` - NSIS / Inno installer, embedded `.ico`.
- `macos/` - Info.plist template, .icns, and the build/sign/notarize
  runbook (`macos/README.md`) for the .app bundle + DragNDrop .dmg.
- `android/` - manifest + Gradle bits, launcher mipmaps.
- `ios/` - Info.plist + bundle config, `Assets.xcassets` app icon.
- `web/` - browser favicons for the wasm shell + hosted-node OCI web root.

`UPDATES.md` is the release-feed / auto-update design (capability dial, signed
`manifest.json` schema, threat model). `UPDATES.json` is only the legacy
QSimpleUpdater-format mirror of that feed (synced by
`scripts/sync-updates-json.sh`).
