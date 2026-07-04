# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# appimagetool + pinned AppImage type2 runtimes, vendored from
# the-distributor/nix/appimagetool.nix (same pins/hashes; wrapper reworked:
# the original's quoted heredoc left `$out` unexpanded and nix's URI-literal
# parsing swallowed `${ARCH:-}`, so runtime injection could never fire).
#
# The wrapper injects --runtime-file with the pinned runtime for the target
# arch unless the caller passes one (CPack does, via
# CPACK_APPIMAGE_RUNTIME_FILE <- DAEMON_APP_APPIMAGE_RUNTIME), and puts the
# helper tools appimagetool shells out to (mksquashfs, zsyncmake,
# desktop-file-validate, ...) on PATH. No network at package time.

{ pkgs }:

let
  inherit (pkgs) stdenv lib cmake pkg-config fetchFromGitHub fetchurl;

  # ---- Vendored type2-runtime binaries (pinned) ----
  # Source: https://github.com/AppImage/type2-runtime/releases
  # Static binaries. The vendored original pinned the "continuous" release
  # (2025-08-11 build), but that tag is re-published in place - upstream
  # rebuilt it on 2026-06-23, so those hashes no longer match what the URL
  # serves. Re-pinned to the immutable tagged release 20251108 instead
  # (SRI hashes via `nix store prefetch-file --executable`).
  type2Runtime = rec {
    version = "20251108";
    x86_64 = fetchurl {
      url = "https://github.com/AppImage/type2-runtime/releases/download/${version}/runtime-x86_64";
      hash = "sha256-csrvWMTqQnQ/iT+qjYpVP2f6M9aiYGYUZyFDrrjDC1U=";
      executable = true;
    };

    aarch64 = fetchurl {
      url = "https://github.com/AppImage/type2-runtime/releases/download/${version}/runtime-aarch64";
      hash = "sha256-vz0WWtbHy6W3PN/AhyU48x+iBmsC+myyPp3Un0Roa8c=";
      executable = true;
    };

    # Helpful for cross-targeting from x86_64/aarch64 hosts
    i686 = fetchurl {
      url = "https://github.com/AppImage/type2-runtime/releases/download/${version}/runtime-i686";
      hash = "sha256-8dpFkelL+wFfRT8yAdqZWqWaVipWQJSQfxl4Lsogwt0=";
      executable = true;
    };

    armhf = fetchurl {
      url = "https://github.com/AppImage/type2-runtime/releases/download/${version}/runtime-armhf";
      hash = "sha256-Tet8cOoLvssoeAT1zNWv2woAiL7o/TZFrRP1ng63hIg=";
      executable = true;
    };

    drv = stdenv.mkDerivation {
      pname = "type2-runtimes";
      inherit version;
      dontUnpack = true;
      installPhase = ''
        mkdir -p $out/share/appimage/type2-runtime
        cp -v ${x86_64}  $out/share/appimage/type2-runtime/runtime-x86_64
        cp -v ${aarch64} $out/share/appimage/type2-runtime/runtime-aarch64
        cp -v ${i686}    $out/share/appimage/type2-runtime/runtime-i686
        cp -v ${armhf}   $out/share/appimage/type2-runtime/runtime-armhf
        chmod +x $out/share/appimage/type2-runtime/*
      '';
      meta = with lib; {
        description = "Pinned AppImage type-2 runtimes for multiple architectures";
        homepage = "https://github.com/AppImage/type2-runtime/releases";
        license = licenses.gpl2Plus;
        platforms = platforms.linux;
      };
    };
  };

  # Arch label of the host platform, appimagetool/type2-runtime naming.
  hostArch =
    {
      x86_64-linux = "x86_64";
      aarch64-linux = "aarch64";
      i686-linux = "i686";
      armv7l-linux = "armhf";
    }
    .${stdenv.hostPlatform.system} or "x86_64";

  # External tools appimagetool invokes at package time.
  runtimeTools = with pkgs; [
    squashfsTools # mksquashfs
    zsync # zsyncmake (used with zsync update information)
    gnupg # gpg for signing via gpgme
    desktop-file-utils # desktop-file-validate
    file
    patchelf
  ];

in
rec {
  appimagetool = stdenv.mkDerivation {
    pname = "appimagetool";
    version = "continuous-2025-10-13-aa0b7dc";

    src = fetchFromGitHub {
      owner = "AppImage";
      repo = "appimagetool";
      rev = "aa0b7dcd6abdd127b11a540c848dd230ebcc5d8b";
      hash = "sha256-/QpTa1BPQ/DpDclkrwYEJZoYaEAyDiXBwr1SgGOiZKw=";
      fetchSubmodules = false;
    };

    nativeBuildInputs = [
      cmake
      pkg-config
    ];

    # Upstream CMake checks for these via pkg-config
    buildInputs = with pkgs; [
      glib # glib-2.0 + gio-2.0
      gpgme
      libgcrypt
      libgpg-error
      curl # libcurl
    ];

    cmakeFlags = [ "-DBUILD_STATIC=OFF" ];

    # Wrap the real binary: helper tools on PATH, pinned runtime injected
    # unless the caller provides --runtime-file.
    postInstall = ''
      mkdir -p $out/libexec
      mv -v $out/bin/appimagetool $out/libexec/appimagetool-real

      cat > $out/bin/appimagetool <<EOF
      #!${pkgs.runtimeShell}
      set -euo pipefail

      export PATH="${lib.makeBinPath runtimeTools}:\$PATH"
      export SSL_CERT_FILE="\''${SSL_CERT_FILE:-${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt}"

      for arg in "\$@"; do
        case "\$arg" in
          --runtime-file|--runtime-file=*)
            exec -a "\$0" "$out/libexec/appimagetool-real" "\$@" ;;
        esac
      done

      # No caller-provided runtime: inject the pinned one for \$ARCH (host
      # arch default), mirroring appimagetool's own ARCH convention.
      case "\''${ARCH:-${hostArch}}" in
        x86_64|amd64|x86-64) runtime_arch=x86_64 ;;
        aarch64|arm64)       runtime_arch=aarch64 ;;
        i686|i386)           runtime_arch=i686 ;;
        armhf|arm|armv7*)    runtime_arch=armhf ;;
        *)
          echo "appimagetool wrapper: unsupported ARCH '\$ARCH'." >&2
          echo "Supported: x86_64, aarch64, i686, armhf (or pass --runtime-file)." >&2
          exit 2 ;;
      esac
      runtime="${type2Runtime.drv}/share/appimage/type2-runtime/runtime-\$runtime_arch"
      exec -a "\$0" "$out/libexec/appimagetool-real" --runtime-file "\$runtime" "\$@"
      EOF
      chmod +x $out/bin/appimagetool
    '';

    meta = with lib; {
      description = "Low-level tool to generate an AppImage from an existing AppDir";
      homepage = "https://github.com/AppImage/appimagetool";
      license = licenses.mit;
      platforms = platforms.linux;
      mainProgram = "appimagetool";
    };
  };

  type2Runtimes = type2Runtime.drv;

  # The pinned runtime file for the host platform - what the flake feeds into
  # CPACK_APPIMAGE_RUNTIME_FILE (via -DDAEMON_APP_APPIMAGE_RUNTIME).
  runtimeFile = "${type2Runtime.drv}/share/appimage/type2-runtime/runtime-${hostArch}";

  appimagetool-full = pkgs.symlinkJoin {
    name = "appimagetool-full";
    paths = [
      appimagetool
      type2Runtime.drv
    ];
  };
}
