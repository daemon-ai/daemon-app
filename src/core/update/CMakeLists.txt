# Release-feed / auto-update (design: packaging/UPDATES.md). The package-time
# capability dial + signed-feed check/verify (Notify), plus the verified download
# and DownloadAndOpen handoff. Both front ends link it.

# --- vendored Monocypher (BLAKE2b-512 + Ed25519 for minisign) ----------------
# Self-contained crypto so signature verification needs no platform TLS (Windows
# ships Schannel-only, the Linux static build dlopens libssl at runtime). Kept in
# its own target WITHOUT the project warnings/tidy interface and exposed as a
# SYSTEM include so the analyzers skip it (mirrors src/core/daemon/codec/vendor).
# The project declares only CXX, so opt the C toolchain in for these TUs.
enable_language(C)
add_library(da_update_monocypher STATIC
    vendor/monocypher/monocypher.c
    vendor/monocypher/monocypher-ed25519.c
)
target_include_directories(da_update_monocypher SYSTEM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/monocypher"
)
set_target_properties(da_update_monocypher PROPERTIES C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)

add_library(da_update STATIC
    update_manager.h
    update_manager.cpp
    minisign_verifier.h
    minisign_verifier.cpp
    semver.h
    semver.cpp
    update_manifest.h
    update_manifest.cpp
    feed_client.h
    feed_client.cpp
    update_downloader.h
    update_downloader.cpp
)
add_library(daemon-app::update ALIAS da_update)

set_target_properties(da_update PROPERTIES AUTOMOC ON)

target_include_directories(da_update PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

# update_manager.cpp includes the generated daemon_app_version.h (currentVersion).
add_dependencies(da_update daemon_app_version_header)

# The per-artifact update-capability dial, FIXED AT PACKAGE TIME (UPDATES.md).
# Passed as a bare enumerator name so a typo fails the compile instead of
# silently degrading. Default None: a build that doesn't opt in has no updater.
set(DAEMON_APP_UPDATE_CAPABILITY "None"
    CACHE STRING "Update capability compiled into this artifact (None|Notify|DownloadAndOpen|SelfApply)"
)
set_property(
    CACHE DAEMON_APP_UPDATE_CAPABILITY
    PROPERTY STRINGS None Notify DownloadAndOpen SelfApply
)
if(NOT DAEMON_APP_UPDATE_CAPABILITY MATCHES "^(None|Notify|DownloadAndOpen|SelfApply)$")
    message(FATAL_ERROR
        "DAEMON_APP_UPDATE_CAPABILITY must be one of None|Notify|DownloadAndOpen|SelfApply, "
        "got '${DAEMON_APP_UPDATE_CAPABILITY}'"
    )
endif()

# The signed-manifest feed URL baked into the artifact (empty = no feed).
set(DAEMON_APP_UPDATE_FEED_URL ""
    CACHE STRING "Signed release-manifest URL compiled into this artifact (empty = no feed)"
)

# The pinned minisign public key (TOFU is rejected; a manifest that does not
# verify against this key is discarded). Set from the package jobs, not defaulted.
set(DAEMON_APP_UPDATE_PUBKEY ""
    CACHE STRING "Pinned minisign public key (base64) this artifact verifies the feed against"
)

# The artifact kind this build ships as (appimage|deb|rpm|nsis|dmg|portable-tar).
# Drives feed artifact selection. The shared Linux binary detects AppImage at
# runtime ($APPIMAGE) and otherwise uses this compiled default.
set(DAEMON_APP_UPDATE_ARTIFACT_KIND ""
    CACHE STRING "Artifact kind compiled into this build (appimage|deb|rpm|nsis|dmg|portable-tar)"
)

target_compile_definitions(da_update PRIVATE
    DAEMON_APP_UPDATE_CAPABILITY=${DAEMON_APP_UPDATE_CAPABILITY}
    DAEMON_APP_UPDATE_FEED_URL="${DAEMON_APP_UPDATE_FEED_URL}"
    DAEMON_APP_UPDATE_PUBKEY="${DAEMON_APP_UPDATE_PUBKEY}"
    DAEMON_APP_UPDATE_ARTIFACT_KIND="${DAEMON_APP_UPDATE_ARTIFACT_KIND}"
)

target_link_libraries(da_update
    PUBLIC
        Qt6::Core
    PRIVATE
        Qt6::Network
        Qt6::Gui
        daemon-app::settings_store
        da_update_monocypher
        daemon-app::version
        daemon-app::warnings
)
