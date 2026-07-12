add_subdirectory(unit)
add_subdirectory(qml)
add_subdirectory(blockeditor)

# i18n coverage gate: fail if any user-facing QML string is assigned a raw
# literal instead of flowing through qsTr(). Runs the source scanner directly
# (no build artifact needed); see scripts/i18n-audit.py + its allowlist.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME i18n_audit
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/i18n-audit.py
                --root ${CMAKE_SOURCE_DIR}/src
                --allowlist ${CMAKE_SOURCE_DIR}/scripts/i18n-audit-allowlist.txt)

    # i18n completeness + cross-locale parity gate: every shipped catalog must be
    # fully translated (no type="unfinished", non-empty, placeholders + edge
    # whitespace preserved) and all catalogs must carry an identical (context,
    # source) message set. Runs the source .ts files directly (no build artifact).
    file(GLOB DAEMON_APP_TS_CATALOGS "${CMAKE_SOURCE_DIR}/i18n/daemon-app_*.ts")
    add_test(
        NAME i18n_translations
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/i18n/check_translations.py
                ${DAEMON_APP_TS_CATALOGS})
else()
    message(STATUS "daemon-app: Python3 not found; skipping the i18n_audit + i18n_translations tests")
endif()

# GUI mock boot smoke: render the full QML root offscreen in mock service mode and
# require a clean exit. The daemon-app binary's offscreen render harness
# (DAEMON_APP_RENDER_SHOTS) boots the root, opens a seeded session, grabs a frame
# per theme, then exits; a QML objectCreationFailed exits -1 (main.cpp). This
# catches QML TypeError/ReferenceError/missing-context-property breakage that the
# unit/QML ctests never see (AD found two such pre-existing breakages this way).
# The TUI counterpart is the daemon_tui_offscreen_* suite. Endgame: also run this
# in the superproject `just e2e` / sim against the sealed bundle.
if(TARGET daemon-app)
    add_test(NAME daemon_app_offscreen_mock_boot
        COMMAND ${CMAKE_COMMAND} -E env
            DAEMON_APP_SERVICE_MODE=mock
            DAEMON_APP_MOCK_SCENARIO=default
            QT_QPA_PLATFORM=offscreen
            QT_QUICK_BACKEND=software
            DAEMON_APP_RENDER_SHOTS=${CMAKE_CURRENT_BINARY_DIR}/mock-boot-shots
            $<TARGET_FILE:daemon-app>
    )
    set_tests_properties(daemon_app_offscreen_mock_boot PROPERTIES TIMEOUT 120)
endif()

# a11y (AT-SPI) coverage gate: launch the app per page under a private
# Xvfb/D-Bus/AT-SPI sandbox and fail if any SHOWING interactive element is
# unnamed (no accessible name / labelled-by / description), plus a static
# blind-spot pass for bespoke MouseArea/TapHandler click targets that never
# enter the AT-SPI tree. The launcher self-manages its own xcb/Xvfb env (so it
# runs even under the offscreen ctest default) and exits 77 (SKIP) when the
# sandbox deps are absent -- a bare checkout outside the devShell skips instead
# of failing. See scripts/a11y-audit.{sh,py} + its allowlist.
add_test(
    NAME a11y_audit
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/a11y-audit.sh
            --binary $<TARGET_FILE:daemon-app>
            --allowlist ${CMAKE_SOURCE_DIR}/scripts/a11y-audit-allowlist.txt)
set_tests_properties(a11y_audit PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)

# TUI transcript-render tests need TuiWidgets (pkg-config), only present when the
# experimental terminal frontend is enabled.
if(DAEMON_APP_TUI)
    add_subdirectory(tui)
endif()
