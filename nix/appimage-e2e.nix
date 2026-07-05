# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
#
# AppImage SelfApply A->B end-to-end test harness (packaging/UPDATES.md,
# "AppImage (SelfApply)"). Two parts:
#
#   mkImage { pubkey, version } - a real, bootable AppImage built like the
#     release one (reuses mkLinuxArtifacts, so daemon-updater is packaged into
#     usr/bin), but pinned to a TEST minisign pubkey and compiled SelfApply, at
#     an arbitrary version. Only the AppImage generator runs (deb/rpm skipped)
#     to keep the E2E build lean. The runner builds it twice (A + B) with a
#     runtime-generated pubkey, so it is invoked via getFlake from the runner.
#
#   runner - the impure `nix run` app that generates the throwaway keypair,
#     builds A + B, serves a signed feed for B on loopback, runs A offscreen
#     with the E2E auto-drive, and asserts the on-disk .AppImage was swapped to
#     B, is executable, and the relaunched B booted. See appimage-e2e-runner.sh.
#
# This is deliberately NOT a flake check: it needs the host's nix + loopback
# networking (mirrors apps.portable-smoke / apps.windows-smoke).
{
  pkgs,
  lib,
  self,
  system,
  mkLinuxArtifacts,
  bootLibPath,
  runtimeFile,
}:
let
  mkImage =
    { pubkey, version }:
    (mkLinuxArtifacts { }).overrideAttrs (old: {
      pname = "daemon-appimage-e2e";
      # Later -D wins: override the shared build's dials with the test pubkey +
      # an explicit version, keeping the SelfApply ceiling.
      cmakeFlags = old.cmakeFlags ++ [
        "-DDAEMON_APP_UPDATE_CAPABILITY=SelfApply"
        "-DDAEMON_APP_UPDATE_PUBKEY=${pubkey}"
        "-DDAEMON_APP_VERSION_STR=${version}"
      ];
      # Only the AppImage generator (the deb/rpm cpack runs are dead weight for
      # this test). daemon-updater still lands in usr/bin via its install rule.
      installPhase = ''
        runHook preInstall
        export HOME="$TMPDIR"
        echo "=== cpack -G AppImage (E2E, version ${version}) ==="
        cpack -G AppImage || {
          find _CPack_Packages -name '*.log' -o -name '*.err' | while read -r f; do
            echo "--- $f ---" >&2
            tail -n 100 "$f" >&2
          done
          exit 1
        }
        mkdir -p "$out"
        ls ./*.AppImage >/dev/null || { echo "no .AppImage produced" >&2; exit 1; }
        cp -v ./*.AppImage "$out"/
        runHook postInstall
      '';
    });

  runner = pkgs.writeShellApplication {
    name = "updater-appimage-e2e";
    # nix itself is intentionally NOT listed: writeShellApplication prepends
    # these to $PATH (it does not clobber it), so the caller's `nix` (the one
    # running `nix run`) is reused for the nested A/B builds - no nix rebuild.
    runtimeInputs = with pkgs; [
      minisign
      python3
      coreutils
      gnugrep
      gnused
      findutils
      squashfsTools # unsquashfs: the type2 runtime can't execute in the sandbox
      patchelf # repoint the payload's interpreter at the store loader
    ];
    text = builtins.replaceStrings
      [ "@self@" "@system@" "@bootLibPath@" "@storeLoader@" "@runtimeFile@" ]
      [
        "${self}"
        system
        bootLibPath
        "${pkgs.glibc}/lib/ld-linux-x86-64.so.2"
        runtimeFile
      ]
      (builtins.readFile ./appimage-e2e-runner.sh);
  };
in
{
  inherit mkImage runner;
}
