# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Qt-for-Android cross toolchain + the daemon-app Android build + APK.
#
# nixpkgs ships no prebuilt Qt-for-Android, so the target stack is
# cross-compiled from source: the SAME qt6 sources as the host Qt (Qt requires
# the host tools' version to match the target version), configured through the
# NDK's CMake toolchain for ONE ABI (arm64-v8a - the only shipping ABI),
# shared, Release. Shared libs are Qt's only supported Android config (the
# QtActivity Java loader dlopens libQt6*.so per res/values/libs.xml; a static
# Android Qt is not supported upstream). The build recipe itself (qtbase
# manual configure + per-module qt-cmake) is the shared qt-from-source.nix
# builder; this file supplies the android flavor's parameters and the APK
# packaging downstream of it.
#
# The SDK/NDK come from androidenv.composeAndroidPackages on the SAME nixpkgs
# pin, evaluated with allowUnfree + android_sdk.accept_license (the `pkgsAndroid`
# import below is scoped to the android outputs only - the default package set
# stays free). The APK is assembled WITHOUT Gradle - Gradle's first run
# resolves plugins from the network, which the nix sandbox forbids - by
# driving androiddeployqt in --aux-mode (stages libs/assets/res + fills Qt's
# template manifests) and then hand-running the classic toolchain:
# javac -> d8 -> aapt2 -> zip -> zipalign -> apksigner (debug key generated at
# build time). devShells.android keeps the stock Gradle path available for
# development (network allowed there).
{
  pkgs,
  # Second import of the same nixpkgs with { allowUnfree = true;
  # android_sdk.accept_license = true; } - ONLY androidenv (SDK/NDK) is taken
  # from it, so everything else shares the free package set's store paths.
  pkgsAndroid,
  appSrc,
  baseVersion,
  versionStr,
  depSources,
}:
let
  inherit (pkgs) lib;

  # --- Android SDK / NDK -----------------------------------------------------
  # Version set mirrors the verified prior art (logos-app) on this same
  # nixpkgs fork: cmdline-tools 12.0, platform-tools 35.0.2, build-tools
  # 35.0.0, platform 35 (compile/target SDK), NDK 27.2 (Qt 6.11's supported
  # LTS NDK line). minSdk rides ANDROID_PLATFORM=android-28 - Qt 6.11's floor
  # (androiddeployqt rejects anything lower).
  ndkVersion = "27.2.12479018";
  androidMinApi = "28";
  androidCompileSdk = "35";
  androidAbi = "arm64-v8a";
  androidTriple = "aarch64-linux-android";
  buildToolsVersion = "35.0.0";

  androidComposition = pkgsAndroid.androidenv.composeAndroidPackages {
    cmdLineToolsVersion = "12.0";
    platformToolsVersion = "35.0.2";
    buildToolsVersions = [ buildToolsVersion ];
    platformVersions = [ androidCompileSdk ];
    includeNDK = true;
    ndkVersions = [ ndkVersion ];
    cmakeVersions = [ "3.22.1" ];
  };
  androidSdkRoot = "${androidComposition.androidsdk}/libexec/android-sdk";
  androidNdkRoot = "${androidSdkRoot}/ndk/${ndkVersion}";
  ndkSysroot = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/sysroot";
  androidJar = "${androidSdkRoot}/platforms/android-${androidCompileSdk}/android.jar";
  buildTools = "${androidSdkRoot}/build-tools/${buildToolsVersion}";

  # Qt 6.11's Android support targets JDK 17 (the gradle template compiles
  # with sourceCompatibility 17); qtbase's own build javac's Qt6Android.jar.
  jdk = pkgs.jdk17;

  qtVersion = pkgs.qt6.qtbase.version;

  # --- Host-tools prefix (QT_HOST_PATH) -----------------------------------
  # Same join as the wasm stack (identical derivation -> shared store path):
  # one host prefix carrying moc/rcc/syncqt/qmlcachegen/qmltyperegistrar/
  # lrelease/qsb + their Qt6*Tools CMake packages.
  qtHost = pkgs.symlinkJoin {
    name = "qt-host-tools-${qtVersion}";
    paths = with pkgs.qt6; [
      qtbase # moc, rcc, syncqt, qmake, Qt6HostInfo
      qtdeclarative # qmlcachegen, qmltyperegistrar, qmlimportscanner
      qttools # lrelease/lupdate (Qt6LinguistTools for qt_add_translations)
      qtshadertools # qsb (Qt6ShaderToolsTools, needed by Quick's shader baking)
    ];
  };

  # Qt's FindEGL/FindGLESv2 cannot probe the NDK sysroot on their own (no
  # pkg-config, re-rooted find paths), so pin them explicitly. Needed by the
  # qtbase configure AND by every module configure: the shared Qt6Gui config
  # re-runs find_dependency(EGL/GLESv2) in each consumer, and an unresolved
  # Qt::Gui makes a module silently configure to "nothing to build" (same
  # failure class portable.nix documents for the static desktop flavor).
  eglGlesFlags = [
    "-DEGL_INCLUDE_DIR=${ndkSysroot}/usr/include"
    "-DEGL_LIBRARY=${ndkSysroot}/usr/lib/${androidTriple}/${androidMinApi}/libEGL.so"
    "-DGLESv2_INCLUDE_DIR=${ndkSysroot}/usr/include"
    "-DGLESv2_LIBRARY=${ndkSysroot}/usr/lib/${androidTriple}/${androidMinApi}/libGLESv2.so"
  ];

  # --- The android Qt stack via the shared builder ---------------------------
  # qt-from-source.nix owns the build recipe (qtbase manual configure, modules
  # through qtbase's qt-cmake + QT_ADDITIONAL_PACKAGES_PREFIX_PATH; under
  # cross-compilation qt-cmake folds those prefixes into CMAKE_FIND_ROOT_PATH);
  # this flavor contributes the NDK toolchain + the android feature matrix.
  #
  # qtbase notes:
  #  * BUILD_SHARED_LIBS=ON explicit: Qt's only supported Android config.
  #  * INPUT_opengl=es2 + EGL/GLES pinned to the NDK sysroot: Qt's FindEGL/
  #    FindGLESv2 probe host-style prefixes and must not leak host GL.
  #  * ANDROID_STL=c++_shared: the Qt libs + app lib share one libc++_shared.so
  #    (androiddeployqt copies it from the NDK per the deployment settings).
  #  * FEATURE_optimize_size=ON like every other from-source flavor (-Os,
  #    global, so the declarative/svg/websockets modules inherit it).
  #  * sql-sqlite ON + system-sqlite OFF (explicit): the app uses
  #    QSqlDatabase/QSQLITE, pin the bundled-sqlite driver like wasm/static.
  #  * No openssl: TLS on Android needs a bundled libssl/libcrypto pair
  #    (upstream ships them via the android_openssl repo, a separate
  #    download); out of scope for the debug APK - ws:// works, wss:// is
  #    documented as a caveat in packaging/android/README.md.
  qtFromSource = import ./qt-from-source.nix { inherit pkgs; } {
    flavor = "android-arm64";
    qtbaseFlags = [
      "-DQT_QMAKE_TARGET_MKSPEC=android-clang"
      "-DCMAKE_TOOLCHAIN_FILE=${androidNdkRoot}/build/cmake/android.toolchain.cmake"
      "-DANDROID_SDK_ROOT=${androidSdkRoot}"
      "-DANDROID_NDK_ROOT=${androidNdkRoot}"
      "-DANDROID_ABI=${androidAbi}"
      "-DANDROID_PLATFORM=android-${androidMinApi}"
      "-DANDROID_STL=c++_shared"
      "-DQT_HOST_PATH=${qtHost}"
      "-DQT_BUILD_TOOLS_WHEN_CROSSCOMPILING=OFF"
      "-DQT_BUILD_EXAMPLES=OFF"
      "-DQT_BUILD_TESTS=OFF"
      "-DQT_GENERATE_SBOM=OFF"
      "-DBUILD_SHARED_LIBS=ON"
      "-DFEATURE_optimize_size=ON"
      "-DINPUT_opengl=es2"
    ]
    ++ eglGlesFlags
    ++ [
      "-DFEATURE_sql_sqlite=ON"
      "-DFEATURE_system_sqlite=OFF"
    ];
    # JDK on top of the builder's cmake+ninja default: qtbase's Android build
    # javac's its Java support jars (Qt6Android.jar) offline at build time.
    qtbaseNativeBuildInputs = with pkgs; [
      cmake
      ninja
      jdk
    ];
    # qtdeclarative also compiles Android jars (QtQuickView bindings).
    moduleNativeBuildInputs = with pkgs; [
      cmake
      ninja
      python3 # qtdeclarative's qml configure requires a host Python
      jdk
    ];
  };
  inherit (qtFromSource) mkQtModule;
  qtbaseAndroid = qtFromSource.qtbase;

  qtshadertoolsAndroid = mkQtModule {
    name = "qtshadertools";
    src = pkgs.qt6.qtshadertools.src;
    extraFlags = eglGlesFlags;
  };

  qtdeclarativeAndroid = mkQtModule {
    name = "qtdeclarative";
    src = pkgs.qt6.qtdeclarative.src;
    qtDeps = [ qtshadertoolsAndroid ];
    # The app pins QQuickStyle::setStyle("Basic") in main() and imports no
    # other style; unlike the static flavors the unused style plugins would at
    # least stay out of the APK (androiddeployqt bundles only scanned
    # imports), but not building them keeps the stack lean and mirrors the
    # wasm/linux-static trim exactly.
    extraFlags = [
      "-DFEATURE_quickcontrols2_fusion=OFF"
      "-DFEATURE_quickcontrols2_imagine=OFF"
      "-DFEATURE_quickcontrols2_material=OFF"
      "-DFEATURE_quickcontrols2_universal=OFF"
    ]
    ++ eglGlesFlags;
  };

  qtsvgAndroid = mkQtModule {
    name = "qtsvg";
    src = pkgs.qt6.qtsvg.src;
    extraFlags = eglGlesFlags;
  };

  qtwebsocketsAndroid = mkQtModule {
    name = "qtwebsockets";
    src = pkgs.qt6.qtwebsockets.src;
    # qtwebsockets ships a QML plugin; Quick pulls shadertools transitively.
    qtDeps = [
      qtdeclarativeAndroid
      qtshadertoolsAndroid
    ];
    extraFlags = eglGlesFlags;
  };

  # One prefix for consumers: `$qtAndroidJoined/bin/qt-cmake` (and the CMake
  # toolchain at lib/cmake/Qt6/qt.toolchain.cmake) sees every module because
  # the toolchain resolves its install prefix from its own (symlinked)
  # location, and CMake does not resolve symlinks for list files.
  qtAndroidJoined = pkgs.symlinkJoin {
    name = "qt-android-${androidAbi}-${qtVersion}";
    paths = [
      qtbaseAndroid
      qtshadertoolsAndroid
      qtdeclarativeAndroid
      qtsvgAndroid
      qtwebsocketsAndroid
    ];
  };

  # --- Third-party for the android app ----------------------------------------
  # MicroTeX (vendored into the app build) links tinyxml2 via pkg-config;
  # nixpkgs' tinyxml-2 is a host library, so cross-compile the same pinned
  # source for the NDK target (mirrors tinyxml2Wasm / tinyxml2Static).
  tinyxml2Android = pkgs.stdenv.mkDerivation {
    pname = "tinyxml2-android-${androidAbi}";
    version = pkgs.tinyxml-2.version;
    src = pkgs.tinyxml-2.src;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
    ];

    dontUseCmakeConfigure = true;

    configurePhase = ''
      runHook preConfigure
      cmake -S . -B build -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=${androidNdkRoot}/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=${androidAbi} \
        -DANDROID_PLATFORM=android-${androidMinApi} \
        -DANDROID_STL=c++_shared \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF \
        -Dtinyxml2_BUILD_TESTING=OFF \
        -DBUILD_TESTING=OFF
      cd build
      runHook postConfigure
    '';
  };

  # KSyntaxHighlighting's find_package(ECM) cannot see host prefixes under the
  # NDK toolchain's CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY; an explicit
  # ECM_DIR bypasses the re-rooted search (ECM is host-arch-independent CMake
  # modules, so using the host copy is correct).
  ecmDir = "${pkgs.kdePackages.extra-cmake-modules}/share/ECM/cmake";

  # Cross-compiling KSyntaxHighlighting needs a NATIVE katehighlightingindexer
  # (it bakes the syntax-definition index into the QRC resources). Identical
  # derivation to the wasm stack's (same inputs -> same store path).
  ksyntaxIndexerHost = pkgs.stdenv.mkDerivation {
    pname = "katehighlightingindexer";
    version = "host";
    src = depSources.ksyntaxhighlighting;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
      perl
      kdePackages.extra-cmake-modules
      qt6.qttools # ecm_install_po_files_as_qm needs Qt6LinguistTools at configure
      qt6.wrapQtAppsHook
    ];
    buildInputs = [ pkgs.qt6.qtbase ];

    cmakeFlags = [
      "-DKSYNTAXHIGHLIGHTING_USE_GUI=OFF"
      "-DBUILD_TESTING=OFF"
    ];
    ninjaFlags = [ "katehighlightingindexer" ]; # only the tool, not the framework

    installPhase = ''
      runHook preInstall
      install -Dm755 bin/katehighlightingindexer "$out/bin/katehighlightingindexer"
      runHook postInstall
    '';
    dontWrapQtApps = true; # links Qt6::Core only; no plugins to wrap for
  };

  # --- daemon-app for android -------------------------------------------------
  # Cross-build against the android Qt stack, then stage the androiddeployqt
  # package tree WITHOUT Gradle (--aux-mode: resolves + copies the Qt libs and
  # QML plugins the app needs, packs the android_rcc_bundle, overlays
  # packaging/android/, and fills Qt's template manifests/libs.xml). The
  # output is the finished package tree the `apk` derivation below assembles
  # and signs.
  #
  # BUILD_SHARED_LIBS=OFF like the wasm build: the vendored KSyntaxHighlighting
  # (KDECMakeSettings defaults it ON) and its QML plugin become static archives
  # linked into libdaemon-app_arm64-v8a.so - App/CMakeLists.txt links the
  # plugin explicitly and main.cpp Q_IMPORT_QML_PLUGINs it on Android, so the
  # APK carries a single app lib instead of loose non-Qt .so's + a shared QML
  # plugin androiddeployqt would need extra wiring to find. Desktop-only
  # inputs are deliberately absent: no qmltermwidget, no qtkeychain (the token
  # store falls back to QSettings), no TUI stack (DAEMON_APP_MOBILE derives ON
  # for ANDROID and gates all of it).
  appStaged = pkgs.stdenv.mkDerivation {
    pname = "daemon-app-android-staged";
    version = baseVersion;
    src = appSrc;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
      pkg-config # MicroTeX resolves tinyxml2 via pkg_check_modules
      # Vendored KSyntaxHighlighting needs ECM + perl on the host (see the
      # desktop package's note); lrelease comes from QT_HOST_PATH's qttools.
      kdePackages.extra-cmake-modules
      perl
      jdk
    ];

    dontUseCmakeConfigure = true;

    configurePhase = ''
      runHook preConfigure
      export PKG_CONFIG_PATH="${tinyxml2Android}/lib/pkgconfig"
      # MicroTeX's CMake links the raw pkg-config library name instead of the
      # PkgConfig:: imported target, so tinyxml2's include/lib dirs never
      # reach its compile/link lines; ride the global flags (same trick as
      # the wasm build - the NDK clang has no nixpkgs cc wrapper either).
      # QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL: Qt's finalization otherwise puts
      # the full androiddeployqt --release run (which ends in Gradle) into the
      # default build; the staging below drives androiddeployqt --aux-mode
      # itself. prepare_apk_dir (the app-lib copy into android-build/libs)
      # stays in ALL.
      ${qtAndroidJoined}/bin/qt-cmake -S . -B build -G Ninja \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DCMAKE_BUILD_TYPE=Release \
        ${lib.concatStringsSep " " eglGlesFlags} \
        "-DCMAKE_CXX_FLAGS=-isystem ${tinyxml2Android}/include" \
        "-DCMAKE_SHARED_LINKER_FLAGS=-L${tinyxml2Android}/lib" \
        "-DCMAKE_MODULE_LINKER_FLAGS=-L${tinyxml2Android}/lib" \
        -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DDAEMON_APP_TUI=OFF \
        -DECM_DIR=${ecmDir} \
        -DKATEHIGHLIGHTINGINDEXER_EXECUTABLE=${ksyntaxIndexerHost}/bin/katehighlightingindexer \
        -DMD4QT_SOURCE_DIR=${depSources.md4qt} \
        -DEARCUT_SOURCE_DIR=${depSources.earcut} \
        -DKSYNTAXHIGHLIGHTING_SOURCE_DIR=${depSources.ksyntaxhighlighting} \
        -DMICROTEX_SOURCE_DIR=${depSources.microtex} \
        -DQRCODEGEN_SOURCE_DIR=${depSources.qrcodegen} \
        -DIMMER_SOURCE_DIR=${depSources.immer} \
        -DDAEMON_APP_VERSION_STR=${versionStr}
      cd build
      runHook postConfigure
    '';

    # ninja (the stdenv hook) builds everything incl. the ${target}_prepare_apk_dir
    # ALL target, which copies libdaemon-app_arm64-v8a.so into the apk dir.
    installPhase = ''
      runHook preInstall

      appdir=src/DaemonApp/App
      settings="$appdir/android-daemon-app-deployment-settings.json"
      test -s "$settings" || { echo "missing deployment settings: $settings" >&2; exit 1; }

      # Stage the full package tree gradle-lessly. --aux-mode runs the whole
      # androiddeployqt pipeline (template copy, ELF dependency walk, Qt lib +
      # QML plugin bundling, rcc resource bundle, packaging/android overlay,
      # manifest/libs.xml placeholder fill) and stops before Gradle.
      ${pkgs.qt6.qtbase}/bin/androiddeployqt \
        --input "$settings" \
        --output "$appdir/android-build" \
        --android-platform android-${androidCompileSdk} \
        --jdk ${jdk.home} \
        --aux-mode \
        --verbose

      mkdir -p "$out"
      cp -r "$appdir/android-build" "$out/android-build"
      cp "$settings" "$out/android-deployment-settings.json"

      runHook postInstall
    '';

    dontFixup = true; # cross .so's; host strip would corrupt, nothing to wrap
  };

  # --- APK assembly (no Gradle) -----------------------------------------------
  # The classic apkbuilder pipeline over the staged tree:
  #   javac (Qt's QtActivity/QtApplication bindings against android.jar +
  #          the staged Qt jars) -> d8 (classes.dex, desugared for minSdk)
  #   aapt2 compile/link (res/ + AndroidManifest.xml -> resources.arsc,
  #          binary manifest; package id/versions come from the manifest that
  #          androiddeployqt already filled)
  #   zip    (classes.dex + lib/<abi>/*.so + assets/, .rcc stored raw - Qt
  #          mmaps the resource bundle straight out of the APK)
  #   zipalign 4 -> apksigner (fresh debug keystore generated per build; see
  #          packaging/android/README.md for the release-signing hook).
  apk = pkgs.stdenv.mkDerivation {
    pname = "daemon-app-apk";
    version = baseVersion;

    dontUnpack = true;

    nativeBuildInputs = [
      jdk
      pkgs.zip
      pkgs.unzip
    ];

    buildPhase = ''
      runHook preBuild

      staged=${appStaged}/android-build
      work=$PWD/apk-work
      mkdir -p "$work"
      cd "$work"

      echo "== javac: Qt Android bindings =="
      # The gradle template compiles qtAndroidDir/src (the QtActivity/
      # QtApplication/QtService bindings shipped in the android Qt prefix)
      # plus the project's own java/ dir; the Qt runtime classes ship
      # prebuilt in the staged libs/*.jar.
      mkdir -p classes
      find ${qtAndroidJoined}/src/android/java -name '*.java' > sources.txt
      if [ -d "$staged/src" ]; then
        find "$staged/src" -name '*.java' >> sources.txt || true
      fi
      jars=$(find "$staged/libs" -name '*.jar' | tr '\n' ':')
      javac -source 17 -target 17 -encoding UTF-8 \
        -classpath "${androidJar}:$jars" \
        -d classes @sources.txt

      echo "== d8: dex the bindings + Qt jars =="
      mapfile -t qtjars < <(find "$staged/libs" -name '*.jar')
      find classes -name '*.class' > class-list.txt
      ${buildTools}/d8 --release \
        --lib ${androidJar} \
        --min-api ${androidMinApi} \
        --output . \
        "''${qtjars[@]}" @class-list.txt
      test -s classes.dex

      echo "== aapt2: compile + link resources =="
      ${buildTools}/aapt2 compile --dir "$staged/res" -o compiled-res.zip
      ${buildTools}/aapt2 link \
        -I ${androidJar} \
        --manifest "$staged/AndroidManifest.xml" \
        --auto-add-overlay \
        -o unaligned.apk \
        compiled-res.zip

      echo "== zip: dex + native libs + assets =="
      zip -q unaligned.apk classes.dex

      # APK layout wants lib/<abi>/, the gradle-convention staging has
      # libs/<abi>/. Compressed entries (extractNativeLibs=true in the
      # manifest): the installer unpacks them like the pre-API-23 default.
      mkdir -p apkroot/lib
      for abidir in "$staged"/libs/*/; do
        abi=$(basename "$abidir")
        mkdir -p "apkroot/lib/$abi"
        find "$abidir" -name '*.so' -exec cp --no-preserve=mode {} "apkroot/lib/$abi/" \;
      done
      cp -r --no-preserve=mode "$staged/assets" apkroot/assets
      # Qt registers the android_rcc_bundle.rcc resource straight out of the
      # APK via the asset fd; it must be STORED (uncompressed), matching the
      # gradle template's `noCompress 'rcc'`.
      (cd apkroot && zip -q -r ../unaligned.apk lib assets -x 'assets/*.rcc')
      (cd apkroot && find assets -name '*.rcc' -print | zip -q -0 ../unaligned.apk -@)

      echo "== zipalign + apksigner (debug key) =="
      ${buildTools}/zipalign -f 4 unaligned.apk aligned.apk
      # Ephemeral debug keystore, regenerated every build: the artifact is
      # explicitly a debug-signed development APK (see the README for the
      # user-provided release keystore hook).
      keytool -genkeypair -v \
        -keystore debug.keystore -storepass android -keypass android \
        -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 \
        -dname "CN=Android Debug,O=Android,C=US"
      ${buildTools}/apksigner sign \
        --ks debug.keystore --ks-pass pass:android --key-pass pass:android \
        --ks-key-alias androiddebugkey \
        --out daemon-app.apk aligned.apk
      ${buildTools}/apksigner verify --print-certs daemon-app.apk

      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      mkdir -p "$out"
      # buildPhase leaves the shell cd'd into the work dir; be explicit.
      cp "$NIX_BUILD_TOP/apk-work/daemon-app.apk" "$out/daemon-app-${baseVersion}-${androidAbi}.apk"
      runHook postInstall
    '';

    dontFixup = true;
  };

  # --- APK structural sanity ----------------------------------------------
  # Asserts with the SDK's own tooling: manifest identity (package id,
  # versionCode/versionName, sdk floor), native lib set (app lib + the Qt
  # runtime + STL), the MicroTeX assets payload, the Qt resource bundle, and
  # a full apksigner verification.
  apkSanity =
    pkgs.runCommand "daemon-apk-sanity"
      {
        nativeBuildInputs = [
          jdk
          pkgs.unzip
        ];
      }
      ''
        apk=$(ls ${apk}/*.apk)
        mkdir -p "$out"
        report="$out/report.txt"
        echo "== apk-sanity: $apk" | tee "$report"

        echo "=== aapt2 dump badging ===" | tee -a "$report"
        ${buildTools}/aapt2 dump badging "$apk" > badging.txt
        head -n 5 badging.txt >> "$report"
        grep -q "package: name='ai.daemon.app'" badging.txt \
          || { echo "FAIL: package id is not ai.daemon.app" >&2; exit 1; }
        grep -q "versionCode='[1-9]" badging.txt \
          || { echo "FAIL: versionCode missing/zero" >&2; exit 1; }
        grep -q "versionName='${baseVersion}" badging.txt \
          || { echo "FAIL: versionName does not start with ${baseVersion}" >&2; exit 1; }
        grep -q "minSdkVersion:'${androidMinApi}'" badging.txt \
          || { echo "FAIL: minSdkVersion is not ${androidMinApi}" >&2; exit 1; }
        grep -q "targetSdkVersion:'${androidCompileSdk}'" badging.txt \
          || { echo "FAIL: targetSdkVersion is not ${androidCompileSdk}" >&2; exit 1; }
        grep -q "application-label:'Daemon'" badging.txt \
          || { echo "FAIL: application label missing" >&2; exit 1; }
        grep -q "launchable-activity: name='org.qtproject.qt.android.bindings.QtActivity'" badging.txt \
          || { echo "FAIL: QtActivity launcher missing" >&2; exit 1; }
        grep -q "native-code: '${androidAbi}'" badging.txt \
          || { echo "FAIL: native-code ABI is not ${androidAbi}" >&2; exit 1; }

        echo "=== payload layout ===" | tee -a "$report"
        unzip -l "$apk" > contents.txt
        for path in \
          classes.dex \
          lib/${androidAbi}/libdaemon-app_${androidAbi}.so \
          lib/${androidAbi}/libQt6Core_${androidAbi}.so \
          lib/${androidAbi}/libQt6Gui_${androidAbi}.so \
          lib/${androidAbi}/libQt6Quick_${androidAbi}.so \
          lib/${androidAbi}/libQt6WebSockets_${androidAbi}.so \
          lib/${androidAbi}/libplugins_platforms_qtforandroid_${androidAbi}.so \
          lib/${androidAbi}/libc++_shared.so \
          assets/android_rcc_bundle.rcc \
          assets/microtex-res/fonts/base/cmex10.ttf \
          assets/microtex-res/SAMPLES.tex; do
          grep -qF " $path" contents.txt \
            || { echo "FAIL: missing in apk: $path" >&2; exit 1; }
        done
        libcount=$(grep -c " lib/${androidAbi}/" contents.txt)
        echo "native libs: $libcount" | tee -a "$report"

        echo "=== apksigner verify ===" | tee -a "$report"
        ${buildTools}/apksigner verify --verbose "$apk" | head -n 4 | tee -a "$report"

        echo "OK" | tee -a "$report"
      '';

  # --- Dev shell -------------------------------------------------------------
  # The stock androiddeployqt Gradle path for development (network allowed
  # outside the nix sandbox: Gradle resolves AGP + androidx from Maven on
  # first run, cached under ~/.gradle). scripts/build-apk-gradle.sh drives
  # the same configure as the appStaged derivation, then androiddeployqt
  # WITHOUT --aux-mode.
  devShell = pkgs.mkShell {
    packages =
      (with pkgs; [
        cmake
        ninja
      ])
      ++ [
        jdk
        androidComposition.androidsdk
        androidComposition.platform-tools
      ];

    shellHook = ''
      export ANDROID_SDK_ROOT=${androidSdkRoot}
      export ANDROID_NDK_ROOT=${androidNdkRoot}
      export JAVA_HOME=${jdk.home}
      export QT_HOST_PATH=${qtHost}
      export DAEMON_APP_QT_ANDROID=${qtAndroidJoined}
      export QT_ANDROID_TOOLCHAIN=${qtAndroidJoined}/lib/cmake/Qt6/qt.toolchain.cmake
      export ANDROIDDEPLOYQT=${pkgs.qt6.qtbase}/bin/androiddeployqt
      # Same vendored-dep wiring as the appStaged derivation, consumed by
      # scripts/build-apk-gradle.sh (and usable with the `android` CMake
      # preset via cmake/toolchains/android.cmake).
      export ECM_DIR="${ecmDir}"
      export DAEMON_APP_KATE_INDEXER="${ksyntaxIndexerHost}/bin/katehighlightingindexer"
      export DAEMON_APP_ANDROID_CXX_FLAGS="-isystem ${tinyxml2Android}/include"
      export DAEMON_APP_ANDROID_LINKER_FLAGS="-L${tinyxml2Android}/lib"
      export MD4QT_SOURCE_DIR="${depSources.md4qt}"
      export EARCUT_SOURCE_DIR="${depSources.earcut}"
      export KSYNTAXHIGHLIGHTING_SOURCE_DIR="${depSources.ksyntaxhighlighting}"
      export MICROTEX_SOURCE_DIR="${depSources.microtex}"
      export QRCODEGEN_SOURCE_DIR="${depSources.qrcodegen}"
      export IMMER_SOURCE_DIR="${depSources.immer}"
    '';
  };
in
{
  inherit
    qtHost
    apk
    apkSanity
    devShell
    ;
  qtbase = qtbaseAndroid;
  qtshadertools = qtshadertoolsAndroid;
  qtdeclarative = qtdeclarativeAndroid;
  qtsvg = qtsvgAndroid;
  qtwebsockets = qtwebsocketsAndroid;
  qtAndroid = qtAndroidJoined;
  tinyxml2 = tinyxml2Android;
  app = appStaged;
}
