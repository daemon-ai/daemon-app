# Android toolchain shim.
#
# Prefer driving Android builds through Qt's own toolchain file, which sets up
# the NDK, ABI and Qt-for-Android paths. Point CMake at it via the environment:
#
#   QT_HOST_PATH        - desktop Qt prefix (for host tools: moc, qmlcachegen...)
#   ANDROID_NDK_ROOT    - Android NDK
#   Qt6_DIR             - Qt for Android CMake package
#
# This file simply forwards to Qt's android toolchain when available.

if(DEFINED ENV{QT_ANDROID_TOOLCHAIN})
    include("$ENV{QT_ANDROID_TOOLCHAIN}")
elseif(DEFINED ENV{Qt6_DIR} AND EXISTS "$ENV{Qt6_DIR}/../../../lib/cmake/Qt6/qt.toolchain.cmake")
    include("$ENV{Qt6_DIR}/../../../lib/cmake/Qt6/qt.toolchain.cmake")
else()
    message(FATAL_ERROR
        "Android toolchain not configured. Set QT_ANDROID_TOOLCHAIN to Qt's "
        "qt.toolchain.cmake (Qt for Android), or provide Qt6_DIR + ANDROID_NDK_ROOT.")
endif()
