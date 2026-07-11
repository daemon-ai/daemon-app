<!--
SPDX-FileCopyrightText: 2026 Jarrad Hope
SPDX-License-Identifier: MPL-2.0
-->

# Icon pipeline

`small.svg` and `large.svg` are the **only** hand-edited icon assets. Every
committed platform icon in the sibling packaging dirs is rasterized from them by
the Nix generator `../../nix/icons.nix`, checked into the tree (so packages never
rasterize at build time), and kept honest by a drift gate — the same model as the
vendored codec.

- `small.svg` — simplified glyph, the source for the small sizes (16/32/48 px).
- `large.svg` — detailed logo, the source for 64 px and up.

Both are square with transparent backgrounds and render full-bleed (no padding
or recolor transforms).

## Regenerate / verify

From the superproject root:

```sh
just update-icons   # regenerate the committed icons from the SVGs (mutates the tree)
just icons-drift    # gate: committed icons match what the SVGs regenerate
```

Or directly from this repo:

```sh
nix run    .#update-icons
nix build  .#checks.$(nix eval --impure --raw --expr builtins.currentSystem).icons-drift -L
```

After editing either SVG, run `just update-icons` and commit the regenerated
binaries alongside the SVG change, or `icons-drift` (part of the packaging gates)
will fail.

## Outputs (size -> source)

| Consumer | Path | Sizes | Source |
| --- | --- | --- | --- |
| Linux hicolor | `linux/icons/daemon-app-{16,32,48}.png` | 16/32/48 | small |
| Linux hicolor | `linux/icons/daemon-app-{64,128,256,512}.png` | 64..512 | large |
| Linux master | `linux/icons/daemon-app.png` | 512 | large |
| Linux scalable | `linux/icons/daemon-app.svg` | vector | large (byte copy) |
| Windows | `windows/daemon-app.ico` | 16/32/48 + 64/128/256 | small + large |
| macOS | `macos/daemon-app.icns` | 16/32/48 + 128/256/512/1024 | small + large |
| Android launcher | `android/res/mipmap-*/ic_launcher.png` | 48/72/96/144/192 | large |
| iOS | `ios/Assets.xcassets/AppIcon.appiconset/icon-1024.png` | 1024 | large |
| Web favicon | `web/favicon-32.png` | 32 | small |
| Web apple-touch | `web/favicon-180.png` | 180 | large |

The 16..256 Linux PNGs are additionally embedded into the desktop executable as a
Qt resource (`:/icons/daemon-app-<N>.png`, see `src/DaemonApp/App/CMakeLists.txt`)
for the window/taskbar and system-tray icon (`platform::appIcon()`), which prefers
the installed hicolor theme entry and falls back to these.

The tools (rsvg-convert, icotool, png2icns) come from the flake-pinned nixpkgs —
there are no host tools.
