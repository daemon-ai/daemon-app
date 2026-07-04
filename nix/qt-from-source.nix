# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Shared Qt-from-source builder: the parameterized core extracted from the
# wasm stack (qt-wasm.nix) so every from-source Qt target - the wasm cross
# build and the linux static desktop stack (portable.nix) - shares ONE build
# recipe instead of forking it.
#
# nixpkgs ships prebuilt Qt only in the host platform's shared desktop
# config; any other config (cross-compiled wasm, statically linked desktop)
# cross-/re-builds the SAME upstream qt6 sources with plain CMake. The shape
# is always:
#
#  * qtbase configured manually (dontUseCmakeConfigure: the cmake hook's
#    -DCMAKE_C_COMPILER=$CC and Nix install-dir flags fight Qt's own
#    toolchain handling), taking the platform's toolchain selection,
#    host-tools handling (QT_HOST_PATH for cross; native builds build their
#    own tools) and feature matrix as ONE ordered flag list, so each flavor
#    documents its complete qtbase surface in its own file;
#  * every further module configured through that qtbase's bin/qt-cmake
#    (which chainloads the recorded toolchain file + host path), sibling
#    modules handed in via QT_ADDITIONAL_PACKAGES_PREFIX_PATH - Qt's
#    supported mechanism for extra module prefixes (it folds them into
#    CMAKE_PREFIX_PATH and, under cross-compilation, CMAKE_FIND_ROOT_PATH).
#    Modules share the common example/test/sbom trims; per-module feature
#    flags ride `extraFlags`.
#
# PARITY GUARD: the wasm consumer must keep producing the exact
# pre-extraction derivations (identical drv hashes - proven at extraction
# time via `nix eval .#...drvPath` before/after). The templates below are
# byte-for-byte the pre-extraction configure phases; treat any edit that
# changes the rendered strings as a wasm rebuild + re-verification.
{ pkgs }:
{
  # Suffixes every pname ("qtbase-<flavor>"): "wasm", "linux-static", ...
  flavor,
  # Which stdenv builds the stack. The wasm cross build keeps plain stdenv
  # (the emscripten toolchain file drives the real compiler); native flavors
  # may pass ccacheStdenv to ride the repo's shared compiler cache.
  stdenv ? pkgs.stdenv,
  # Shell fragment exported at the top of every configure phase (the wasm
  # emscripten cache/EMSDK env; empty for native builds).
  buildEnv ? "",
  # Everything after -DCMAKE_BUILD_TYPE on the qtbase configure line, as an
  # ordered list (toolchain + host tools + trims + feature matrix).
  qtbaseFlags,
  # Pristine upstream tarball by default: nixpkgs' desktop patch set targets
  # the shared nix-store layout and does not apply to these configs.
  qtbaseSrc ? pkgs.qt6.qtbase.src,
  qtbaseNativeBuildInputs ? (
    with pkgs;
    [
      cmake
      ninja
    ]
  ),
  # Target libraries for qtbase (X11/wayland/fontconfig/GL on linux; the
  # wasm cross build resolves everything from the emscripten sysroot).
  qtbaseBuildInputs ? [ ],
  moduleNativeBuildInputs ? (
    with pkgs;
    [
      cmake
      ninja
      python3 # qtdeclarative's qml configure requires a host Python
    ]
  ),
  # Target libraries every module of the flavor needs on top of qtbase. A
  # STATIC qtbase's Qt6Gui exports find_dependency(OpenGL/XCB/...) for its
  # private deps - consumers must resolve them again at configure time - so
  # the linux-static flavor hands the platform floor to every module. The
  # wasm sysroot needs nothing here.
  moduleBuildInputs ? [ ],
}:
let
  inherit (pkgs) lib;

  # Qt requires the host tools' version to match the target version, so
  # every from-source stack tracks nixpkgs' qt6 release.
  qtVersion = pkgs.qt6.qtbase.version;

  qtbase = stdenv.mkDerivation {
    pname = "qtbase-${flavor}";
    version = qtVersion;
    src = qtbaseSrc;

    nativeBuildInputs = qtbaseNativeBuildInputs;
    buildInputs = qtbaseBuildInputs;

    # The cmake hook would inject -DCMAKE_C_COMPILER=$CC and Nix install-dir
    # flags, both of which fight Qt's own toolchain handling; configure
    # manually instead. ninja's hook still drives build + install.
    dontUseCmakeConfigure = true;

    configurePhase = ''
      runHook preConfigure
      ${buildEnv}
      cmake -S . -B build -G Ninja \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DCMAKE_BUILD_TYPE=Release \
        ${lib.concatStringsSep " \\\n  " qtbaseFlags}
      cd build
      runHook postConfigure
    '';
  };

  # A Qt module on top of this stack's qtbase. `qtDeps` are sibling modules
  # of the same flavor (each in its own store prefix); `extraFlags` carry
  # the per-module feature trims; `buildInputs` adds module-specific target
  # libs on top of the flavor-wide moduleBuildInputs.
  mkQtModule =
    {
      name,
      src,
      qtDeps ? [ ],
      extraFlags ? [ ],
      buildInputs ? [ ],
    }:
    stdenv.mkDerivation {
      pname = "${name}-${flavor}";
      inherit src;
      version = qtVersion;
      buildInputs = moduleBuildInputs ++ buildInputs;

      nativeBuildInputs = moduleNativeBuildInputs;

      dontUseCmakeConfigure = true;

      configurePhase = ''
        runHook preConfigure
        ${buildEnv}
        export QT_ADDITIONAL_PACKAGES_PREFIX_PATH="${lib.concatStringsSep ":" (map toString qtDeps)}"
        ${qtbase}/bin/qt-cmake -S . -B build -G Ninja \
          -DCMAKE_INSTALL_PREFIX="$out" \
          -DCMAKE_BUILD_TYPE=Release \
          -DQT_BUILD_EXAMPLES=OFF \
          -DQT_BUILD_TESTS=OFF \
          -DQT_GENERATE_SBOM=OFF \
          ${lib.concatStringsSep " " extraFlags}
        cd build
        runHook postConfigure
      '';
    };
in
{
  inherit qtVersion qtbase mkQtModule;
}
