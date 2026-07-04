#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Regenerate the committed raster icon set from packaging/linux/icons/daemon-app.svg:
#   - hicolor PNGs (16..512) next to this script (daemon-app-<N>.png)
#   - packaging/linux/icons/daemon-app.png       (512px master)
#   - packaging/windows/daemon-app.ico           (multi-size, icotool)
#   - packaging/macos/daemon-app.icns            (bonus; real macOS assets land
#                                                 with the macOS workstream)
#   - packaging/android/res/mipmap-*/ic_launcher.png (Android launcher
#     densities; referenced by packaging/android/AndroidManifest.xml)
#
# The rasters are committed so packages never rasterize at build time. Tools
# come from the flake-pinned nixpkgs (librsvg / icoutils / libicns); there are
# no host tools on purpose.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../../.." && pwd)"
svg="$here/daemon-app.svg"

run() {
  nix shell --inputs-from "$repo" \
    nixpkgs#librsvg nixpkgs#icoutils nixpkgs#libicns \
    --command "$@"
}

sizes=(16 32 48 64 128 256 512)
for s in "${sizes[@]}"; do
  run rsvg-convert -w "$s" -h "$s" -o "$here/daemon-app-$s.png" "$svg"
  echo "wrote daemon-app-$s.png"
done
cp "$here/daemon-app-512.png" "$here/daemon-app.png"
echo "wrote daemon-app.png (512 master)"

# Windows .ico: every size up to 256 (the ICO format ceiling).
run icotool -c -o "$repo/packaging/windows/daemon-app.ico" \
  "$here"/daemon-app-{16,32,48,64,128,256}.png
echo "wrote packaging/windows/daemon-app.ico"

# Bonus .icns while libicns is at hand; 512/256/128/32/16 map to icns types.
run png2icns "$repo/packaging/macos/daemon-app.icns" \
  "$here"/daemon-app-{16,32,128,256,512}.png
echo "wrote packaging/macos/daemon-app.icns"

# Android launcher mipmaps: one ic_launcher.png per density bucket (the
# framework picks by display density; 48dp baseline x the bucket scale).
declare -A android_densities=(
  [mdpi]=48 [hdpi]=72 [xhdpi]=96 [xxhdpi]=144 [xxxhdpi]=192
)
for d in mdpi hdpi xhdpi xxhdpi xxxhdpi; do
  s=${android_densities[$d]}
  mkdir -p "$repo/packaging/android/res/mipmap-$d"
  run rsvg-convert -w "$s" -h "$s" \
    -o "$repo/packaging/android/res/mipmap-$d/ic_launcher.png" "$svg"
  echo "wrote packaging/android/res/mipmap-$d/ic_launcher.png"
done
