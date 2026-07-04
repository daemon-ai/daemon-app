# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# CMake 4.2.x for the CPack AppImage generator (added in 4.2; the pinned
# nixpkgs ships 4.1.2). Overrides the nixpkgs cmake derivation with the newer
# source instead of a from-scratch package - far less code, and the nixpkgs
# setup hooks / configure flags carry over unchanged.
#
# Patch status against 4.2.7 (verified by dry-applying):
#   - nixpkgs-cmake-prefix-path.patch  applies (NIXPKGS_CMAKE_PREFIX_PATH)
#   - add-nixpkgs-libc-paths.patch     applies (Nix libc include/lib dirs)
#   - remove-impure-search-paths.patch DROPPED: conflicts with 4.2 source
#     (FindBLAS/FindCUDAToolkit/FindJNI/FindPerlLibs/cmcurl hunks moved).
#     It only trims /usr-style fallback search paths - harmless inside the
#     build sandbox, where those paths do not exist.

{ pkgs }:

pkgs.cmake.overrideAttrs (old: {
  version = "4.2.7";
  src = pkgs.fetchurl {
    url = "https://cmake.org/files/v4.2/cmake-4.2.7.tar.gz";
    hash = "sha256-nIRqSeeD0cLTHvyOf9ynKGHkNBmIekRSGkuDenFOnEo=";
  };
  patches = builtins.filter (
    p: !(pkgs.lib.hasSuffix "remove-impure-search-paths.patch" (baseNameOf p))
  ) (old.patches or [ ]);
})
