{
  description = "daemon-app - Qt 6 QML/C++ development environment, build, and vendored dependencies";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";

    # --- Content renderer dependencies (all platforms; wired when the
    #     markdown/diagram renderer module lands) ---
    md4qt = {
      url = "gitlab:libraries/md4qt/345bba20c225c105dc9050dbb150bcc0d4c0c84f?host=invent.kde.org";
      flake = false;
    };
    earcut = {
      url = "git+https://github.com/mapbox/earcut.hpp?allRefs=1&rev=64530eabb07cfbdf48b2577a763ca26f1afc0b76&submodules=1";
      flake = false;
    };
    ksyntaxhighlighting = {
      url = "gitlab:frameworks/syntax-highlighting/ccb31f722406d5b980ba57cf71a3ffab70a82847?host=invent.kde.org";
      flake = false;
    };

    # --- Desktop-only dependencies ---
    qwindowkit = {
      url = "git+https://github.com/stdware/qwindowkit?allRefs=1&rev=c783b89d119a672b0aa4262b520c0783d0612689&submodules=1";
      flake = false;
    };
    qsimpleupdater = {
      url = "github:alex-spataru/QSimpleUpdater/6c78600eab46e307eb0706a52b847516eb72a146";
      flake = false;
    };
    qautostart = {
      url = "github:Skycoder42/QAutoStart/a27a028f0ac9e49a0c90732d7ea090e2bd31c2c3";
      flake = false;
    };
    qxtglobalshortcut = {
      url = "github:hluk/qxtglobalshortcut/0e47e8c6bddfe42ecb1f3706ead805d3abc56a7e";
      flake = false;
    };
  };

  outputs =
    { nixpkgs, flake-utils, md4qt, earcut, ksyntaxhighlighting, qwindowkit, qsimpleupdater, qautostart, qxtglobalshortcut, ... }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        # Shared on-disk cache for C/C++ builds (ccache).
        # Expose it to the build sandbox with:
        #   nix build .# --extra-sandbox-paths /var/cache/ccache
        ccacheDir = "/var/cache/ccache";

        ccacheOverlay = final: prev: {
          ccacheWrapper = prev.ccacheWrapper.override {
            extraConfig = ''
              export CCACHE_COMPRESS=1
              export CCACHE_DIR="${ccacheDir}"
              export CCACHE_UMASK=007
              export CCACHE_SLOPPINESS=random_seed
            '';
          };
        };

        pkgs = import nixpkgs {
          inherit system;
          overlays = [ ccacheOverlay ];
        };

        qtPackages = with pkgs.qt6; [
          qtbase          # Core, Gui, Widgets, Network, Sql
          qtdeclarative   # Qml, Quick, QuickControls2, QuickTest
          qttools
          qtshadertools
          qtsvg
        ];

        # Source dirs exported to CMake as <DEP>_SOURCE_DIR.
        depFlags = [
          "-DMD4QT_SOURCE_DIR=${md4qt}"
          "-DEARCUT_SOURCE_DIR=${earcut}"
          "-DKSYNTAXHIGHLIGHTING_SOURCE_DIR=${ksyntaxhighlighting}"
          "-DQWINDOWKIT_SOURCE_DIR=${qwindowkit}"
          "-DQSIMPLEUPDATER_SOURCE_DIR=${qsimpleupdater}"
          "-DQAUTOSTART_SOURCE_DIR=${qautostart}"
          "-DQXTGLOBALSHORTCUT_SOURCE_DIR=${qxtglobalshortcut}"
        ];

        daemon-app = pkgs.ccacheStdenv.mkDerivation {
          pname = "daemon-app";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            # KSyntaxHighlighting builds from source via add_subdirectory: it needs
            # ECM (find_package(ECM REQUIRED)) and perl (find_package(Perl REQUIRED),
            # used to auto-generate the PHP syntax definition at build time). The
            # top-level extra-cmake-modules alias was removed with KDE5; the Qt6 ECM
            # lives under kdePackages.
            kdePackages.extra-cmake-modules
            perl
            qt6.wrapQtAppsHook
          ];

          buildInputs = qtPackages;

          cmakeFlags = depFlags;
        };
      in
      {
        packages.default = daemon-app;

        apps.default = {
          type = "app";
          program = "${daemon-app}/bin/daemon-app";
          meta.description = "Run the daemon-app Qt Quick application";
        };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            cmake
            ninja
            pkg-config
            clang-tools
            gcc
            gdb
            kdePackages.extra-cmake-modules
            perl
          ] ++ qtPackages;

          shellHook = ''
            export QT_PLUGIN_PATH="${pkgs.lib.makeSearchPath pkgs.qt6.qtbase.qtPluginPrefix qtPackages}:$QT_PLUGIN_PATH"
            export QML_IMPORT_PATH="${pkgs.lib.makeSearchPath pkgs.qt6.qtbase.qtQmlPrefix qtPackages}:$QML_IMPORT_PATH"
            export QML2_IMPORT_PATH="$QML_IMPORT_PATH:$QML2_IMPORT_PATH"
            export CMAKE_PREFIX_PATH="${pkgs.lib.makeSearchPath "lib/cmake" qtPackages}:$CMAKE_PREFIX_PATH"
            export MD4QT_SOURCE_DIR="${md4qt}"
            export EARCUT_SOURCE_DIR="${earcut}"
            export KSYNTAXHIGHLIGHTING_SOURCE_DIR="${ksyntaxhighlighting}"
            export QWINDOWKIT_SOURCE_DIR="${qwindowkit}"
            export QSIMPLEUPDATER_SOURCE_DIR="${qsimpleupdater}"
            export QAUTOSTART_SOURCE_DIR="${qautostart}"
            export QXTGLOBALSHORTCUT_SOURCE_DIR="${qxtglobalshortcut}"
          '';
        };
      }
    );
}
