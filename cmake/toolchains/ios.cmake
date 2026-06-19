# iOS toolchain shim.
#
# Forwards to Qt's iOS toolchain file (which configures the Xcode generator,
# SDK and code-signing hooks). Provide Qt for iOS via Qt6_DIR / QT_HOST_PATH.

if(DEFINED ENV{QT_IOS_TOOLCHAIN})
    include("$ENV{QT_IOS_TOOLCHAIN}")
elseif(DEFINED ENV{Qt6_DIR} AND EXISTS "$ENV{Qt6_DIR}/../../../lib/cmake/Qt6/qt.toolchain.cmake")
    include("$ENV{Qt6_DIR}/../../../lib/cmake/Qt6/qt.toolchain.cmake")
else()
    message(FATAL_ERROR
        "iOS toolchain not configured. Set QT_IOS_TOOLCHAIN to Qt's "
        "qt.toolchain.cmake (Qt for iOS), or provide Qt6_DIR + QT_HOST_PATH.")
endif()
