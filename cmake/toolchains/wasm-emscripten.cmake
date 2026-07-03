# Qt for WebAssembly toolchain shim.
#
# The Nix wasm shell/package exports QT_WASM_TOOLCHAIN to the target Qt
# qt.toolchain.cmake. QT_HOST_PATH points at the matching native Qt used for
# host tools such as moc, rcc, qmlcachegen, and lrelease.

if(DEFINED ENV{QT_HOST_PATH} AND NOT DEFINED QT_HOST_PATH)
    set(QT_HOST_PATH "$ENV{QT_HOST_PATH}" CACHE PATH "Host Qt prefix for Qt wasm builds")
endif()

if(DEFINED ENV{QT_WASM_TOOLCHAIN})
    include("$ENV{QT_WASM_TOOLCHAIN}")
elseif(DEFINED ENV{Qt6_DIR} AND EXISTS "$ENV{Qt6_DIR}/../../../lib/cmake/Qt6/qt.toolchain.cmake")
    include("$ENV{Qt6_DIR}/../../../lib/cmake/Qt6/qt.toolchain.cmake")
else()
    message(FATAL_ERROR
        "Qt WebAssembly toolchain not configured. Enter `nix develop .#wasm` "
        "or set QT_WASM_TOOLCHAIN to Qt's wasm qt.toolchain.cmake.")
endif()
