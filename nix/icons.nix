# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Icon pipeline: rasterize the two hand-authored source SVGs
# (packaging/icons/{small,large}.svg) into every platform icon format the
# packaging + runtime consumers need, byte-for-byte reproducibly.
#
# The outputs are COMMITTED into the working tree (via `nix run .#update-icons`)
# at the exact paths the build already reads - so packages never rasterize at
# build time - and kept honest by `checks.icons-drift` (same model as the
# vendored codec: a pure generator + an explicit update step + a drift gate).
#
# Source-selection rule (transparent, square, full-bleed - no padding/recolor):
#   * sizes <= 48 px render from small.svg (the simplified glyph);
#   * sizes >= 64 px render from large.svg (the detailed logo).
# Exceptions the size rule alone does not capture:
#   * Android launcher mipmaps always use large.svg (even the 48 px mdpi bucket
#     is a physically large launcher icon), and
#   * the scalable hicolor icon (packaging/linux/icons/daemon-app.svg) is a
#     byte copy of large.svg.
#
# The derivation writes a manifest.txt of every emitted path (relative to the
# repo root) so update-icons / icons-drift iterate it instead of duplicating the
# file list in Nix.

{ pkgs, iconSrc }:

let
  inherit (pkgs) lib;

  # Tools from the pinned nixpkgs (no host tools): rsvg-convert (rasterize),
  # icotool (multi-size .ico), png2icns (multi-size .icns).
  tools = with pkgs; [
    librsvg # rsvg-convert
    icoutils # icotool
    libicns # png2icns
    coreutils
  ];

in
pkgs.runCommand "daemon-app-icons"
  {
    nativeBuildInputs = tools;
    passthru = { inherit iconSrc; };
  }
  ''
    set -euo pipefail

    small="${iconSrc}/small.svg"
    large="${iconSrc}/large.svg"

    # Scratch dir for the intermediate PNG renders (only a subset is committed
    # directly; the rest feed icotool / png2icns).
    tmp="$(mktemp -d)"

    # render <src-svg> <size> -> $tmp/<size>.png (transparent, square, full-bleed)
    render() {
      local src="$1" size="$2"
      rsvg-convert -w "$size" -h "$size" -o "$tmp/$size.png" "$src"
    }

    # Every size the consumers need, from its rule-selected source.
    for s in 16 32 48; do render "$small" "$s"; done
    for s in 64 72 96 128 144 180 192 256 512 1024; do render "$large" "$s"; done

    manifest="$out/manifest.txt"
    mkdir -p "$out"
    : > "$manifest"

    # emit <relpath> <src-file>: copy a generated file into $out at its repo-
    # relative path and record it in the manifest.
    emit() {
      local rel="$1" src="$2"
      install -D -m644 "$src" "$out/$rel"
      echo "$rel" >> "$manifest"
    }

    # --- Linux hicolor PNG set + 512 master + scalable SVG ------------------
    for s in 16 32 48 64 128 256 512; do
      emit "packaging/linux/icons/daemon-app-$s.png" "$tmp/$s.png"
    done
    emit "packaging/linux/icons/daemon-app.png" "$tmp/512.png"
    emit "packaging/linux/icons/daemon-app.svg" "$large"

    # --- Windows .ico (multi-size up to 256, the ICO ceiling) --------------
    icotool -c -o "$tmp/daemon-app.ico" \
      "$tmp/16.png" "$tmp/32.png" "$tmp/48.png" \
      "$tmp/64.png" "$tmp/128.png" "$tmp/256.png"
    emit "packaging/windows/daemon-app.ico" "$tmp/daemon-app.ico"

    # --- macOS .icns (classic + retina slots png2icns maps by dimension) ---
    png2icns "$tmp/daemon-app.icns" \
      "$tmp/16.png" "$tmp/32.png" "$tmp/48.png" \
      "$tmp/128.png" "$tmp/256.png" "$tmp/512.png" "$tmp/1024.png"
    emit "packaging/macos/daemon-app.icns" "$tmp/daemon-app.icns"

    # --- Android launcher mipmaps (density buckets, all from large.svg) -----
    emit "packaging/android/res/mipmap-mdpi/ic_launcher.png"    "$tmp/48.png"
    emit "packaging/android/res/mipmap-hdpi/ic_launcher.png"    "$tmp/72.png"
    emit "packaging/android/res/mipmap-xhdpi/ic_launcher.png"   "$tmp/96.png"
    emit "packaging/android/res/mipmap-xxhdpi/ic_launcher.png"  "$tmp/144.png"
    emit "packaging/android/res/mipmap-xxxhdpi/ic_launcher.png" "$tmp/192.png"

    # --- iOS asset catalog (single 1024 universal icon; Xcode 14+) ----------
    icondir="packaging/ios/Assets.xcassets/AppIcon.appiconset"
    emit "$icondir/icon-1024.png" "$tmp/1024.png"
    # printf (not a heredoc): a heredoc terminator cannot be indented inside
    # this Nix indented string, and a flat literal keeps the JSON byte-stable.
    printf '%s\n' \
      '{' \
      '  "images" : [' \
      '    {' \
      '      "filename" : "icon-1024.png",' \
      '      "idiom" : "universal",' \
      '      "platform" : "ios",' \
      '      "size" : "1024x1024"' \
      '    }' \
      '  ],' \
      '  "info" : {' \
      '    "author" : "xcode",' \
      '    "version" : 1' \
      '  }' \
      '}' > "$tmp/Contents.json"
    emit "$icondir/Contents.json" "$tmp/Contents.json"

    # --- Web favicons (browser tab + apple-touch) ---------------------------
    emit "packaging/web/favicon-32.png"  "$tmp/32.png"
    emit "packaging/web/favicon-180.png" "$tmp/180.png"

    echo "generated $(wc -l < "$manifest") icon files"
  ''
