# Translation wiring for the GUI (daemon-app) and TUI (daemon-tui).
#
# qt_add_translations (Qt 6.11) drives lupdate/lrelease and embeds the compiled
# .qm files into the linked targets as a Qt resource under "/i18n". The runtime
# loads them with a QTranslator (see src/DaemonApp/App/main.cpp and
# src/tui/main.cpp). The same catalog backs both frontends so shared C++ view
# model strings are translated once.

# Locales shipped with the app. en_US is the source language (see the
# -source-language lupdate option below), so it has no .ts file - the strings
# live in the qsTr()/tr() literals. Each shipped language has a committed
# daemon-app_<code>.ts catalog here; add a language by translating a new
# daemon-app_<code>.ts and listing <code> below (and in i18n::availableLocales()).
set(DAEMON_APP_LOCALES
    pt_BR
    id
    tr
    hi
    zh_CN
    de
    fil
    zh_TW
    ru
    es
    ja
    bn
)

# Source files lupdate scans for qsTr()/tr(). Globbed across the whole tree so
# new strings are picked up on the next `update_translations` run; vendored and
# generated code is excluded (it carries no user-facing, translatable text).
file(GLOB_RECURSE DAEMON_APP_I18N_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.qml"
)
list(FILTER DAEMON_APP_I18N_SOURCES EXCLUDE REGEX
    "/(vendor|generated)/|/codec/vendor/"
)

set(DAEMON_APP_TS_FILES "")
foreach(loc IN LISTS DAEMON_APP_LOCALES)
    list(APPEND DAEMON_APP_TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/i18n/daemon-app_${loc}.ts")
endforeach()

# Embed into every frontend that exists in this configuration. The TUI target
# is only defined when DAEMON_APP_TUI is ON (see the Nix flake's `tui` package).
set(DAEMON_APP_I18N_TARGETS daemon-app)
if(TARGET daemon-tui)
    list(APPEND DAEMON_APP_I18N_TARGETS daemon-tui)
endif()

qt_add_translations(
    TARGETS ${DAEMON_APP_I18N_TARGETS}
    SOURCES ${DAEMON_APP_I18N_SOURCES}
    TS_FILES ${DAEMON_APP_TS_FILES}
    RESOURCE_PREFIX "/i18n"
    # We keep plural forms in the per-locale catalogs; no separate native
    # plurals-only file is needed.
    NO_GENERATE_PLURALS_TS_FILE
    LUPDATE_OPTIONS -no-obsolete -locations none -source-language en_US
)
