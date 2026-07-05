// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/update_manager.h"

#include "daemon_app_version.h"
#include "settings/isettings_store.h"
#include "update/feed_client.h"
#include "update/minisign_verifier.h"
#include "update/semver.h"
#include "update/update_downloader.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>
#include <QMetaEnum>
#include <QNetworkAccessManager>
#include <QSysInfo>

// Package-time defines (see src/core/update/CMakeLists.txt). The capability dial
// arrives as a bare enumerator name (compile error on a typo); the rest arrive as
// quoted string literals.
#ifndef DAEMON_APP_UPDATE_CAPABILITY
#define DAEMON_APP_UPDATE_CAPABILITY None
#endif
#ifndef DAEMON_APP_UPDATE_FEED_URL
#define DAEMON_APP_UPDATE_FEED_URL ""
#endif
#ifndef DAEMON_APP_UPDATE_PUBKEY
#define DAEMON_APP_UPDATE_PUBKEY ""
#endif
#ifndef DAEMON_APP_UPDATE_ARTIFACT_KIND
#define DAEMON_APP_UPDATE_ARTIFACT_KIND ""
#endif
#ifndef DAEMON_APP_UPDATE_CHANNEL
#define DAEMON_APP_UPDATE_CHANNEL "stable"
#endif

namespace update {

namespace {

constexpr int kInitialCheckDelayMs = 15 * 1000;      // check-on-start after ~15s
constexpr int kPollIntervalMs = 24 * 60 * 60 * 1000; // then daily
constexpr const char* kProduct = "daemon";

// Settings keys for poll bookkeeping.
constexpr QLatin1StringView kKeyLastCheck("update/lastCheck");
constexpr QLatin1StringView kKeyEtag("update/etag");
constexpr QLatin1StringView kKeyDismissed("update/dismissedVersion");
constexpr QLatin1StringView kKeyAutoCheck("update/autoCheck");
constexpr QLatin1StringView kKeyLatest("update/latestVersion");
constexpr QLatin1StringView kKeyNotes("update/notesUrl");

UpdateManager::Capability minCapability(UpdateManager::Capability a, UpdateManager::Capability b) {
    return static_cast<int>(a) < static_cast<int>(b) ? a : b;
}

// The build's target OS token, matching the manifest `os` enum.
QString buildOs() {
#if defined(Q_OS_ANDROID)
    return QStringLiteral("android");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#elif defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux");
#else
    return {};
#endif
}

// The host CPU arch, mapped to the manifest `arch` enum ("arm64" -> "aarch64").
QString buildArch() {
    const QString arch = QSysInfo::currentCpuArchitecture();
    if (arch == QStringLiteral("arm64")) {
        return QStringLiteral("aarch64");
    }
    return arch; // "x86_64" passes through
}

// The artifact kind this build installs. One shared Linux binary feeds
// AppImage/deb/rpm/tar, so the AppImage case is detected at runtime via $APPIMAGE
// (set by the AppImage runtime); otherwise the compiled default is used.
QString buildArtifactKind() {
    if (!qEnvironmentVariableIsEmpty("APPIMAGE")) {
        return QStringLiteral("appimage");
    }
    return QStringLiteral(DAEMON_APP_UPDATE_ARTIFACT_KIND);
}

} // namespace

UpdateManager::UpdateManager(QObject* parent)
    : QObject(parent), m_capability(Capability::DAEMON_APP_UPDATE_CAPABILITY),
      m_effectiveCapability(Capability::DAEMON_APP_UPDATE_CAPABILITY) {
    m_pollTimer.setInterval(kPollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, [this] {
        if (autoCheck()) {
            check();
        }
    });
}

UpdateManager::~UpdateManager() = default;

void UpdateManager::setSettings(settings::ISettingsStore* settings) {
    m_settings = settings;
    // Restore a previously-offered version so a 304 (unchanged manifest) can
    // re-surface the banner without re-parsing.
    if (m_settings != nullptr) {
        m_latestVersion = m_settings->value(kKeyLatest, QString()).toString();
        m_notesUrl = m_settings->value(kKeyNotes, QString()).toString();
    }
    // Staging hygiene: drop superseded version dirs + stale .part temps.
    UpdateDownloader::cleanupStale(semver::stripBuildMetadata(currentVersion()));
    armPolling();
}

QString UpdateManager::currentVersion() const {
    return QStringLiteral(DAEMON_APP_VERSION_STR);
}

QUrl UpdateManager::resolvedFeedUrl() const {
    // The compiled feed is the trust-relevant default. A runtime override is only
    // honored on a non-inert build (content is still gated by the pinned
    // signature); it exists for the E2E harness to point at a local signed feed.
    if (m_capability != Capability::None) {
        const QString override = qEnvironmentVariable("DAEMON_APP_UPDATE_FEED_URL_OVERRIDE");
        if (!override.isEmpty()) {
            return QUrl(override);
        }
    }
    return QUrl(QStringLiteral(DAEMON_APP_UPDATE_FEED_URL));
}

QUrl UpdateManager::feedUrl() const {
    return resolvedFeedUrl();
}

QString UpdateManager::capabilityToString(Capability capability) {
    // valueToKey(int) is QT_CORE_REMOVED_SINCE(6,9); only the quint64 overload is visible.
    return QString::fromLatin1(
        QMetaEnum::fromType<Capability>().valueToKey(static_cast<quint64>(capability)));
}

QString UpdateManager::stateName() const {
    return QString::fromLatin1(
        QMetaEnum::fromType<State>().valueToKey(static_cast<quint64>(m_state)));
}

bool UpdateManager::canDownload() const {
    return m_state == State::UpdateAvailable &&
           m_effectiveCapability >= Capability::DownloadAndOpen && m_haveSelected;
}

UpdateManager::Capability UpdateManager::capabilityFromString(const QString& name) {
    bool ok = false;
    const int value = QMetaEnum::fromType<Capability>().keyToValue(name.toLatin1(), &ok);
    // Fail closed: anything unrecognized is the inert dial, never an escalation.
    return ok ? static_cast<Capability>(value) : Capability::None;
}

bool UpdateManager::dismissed() const {
    if (m_settings == nullptr || m_latestVersion.isEmpty()) {
        return false;
    }
    return m_settings->value(kKeyDismissed, QString()).toString() == m_latestVersion;
}

bool UpdateManager::autoCheck() const {
    if (m_settings == nullptr) {
        return true; // default on
    }
    return m_settings->value(kKeyAutoCheck, true).toBool();
}

void UpdateManager::setAutoCheck(bool on) {
    if (m_settings == nullptr || autoCheck() == on) {
        return;
    }
    m_settings->setValue(kKeyAutoCheck, on);
    // Take effect immediately: (re)arm the daily poll when enabled, stop it when
    // disabled (manual check() still works regardless).
    if (on) {
        armPolling();
    } else {
        m_pollTimer.stop();
    }
    emit autoCheckChanged();
}

void UpdateManager::dismiss() {
    if (m_settings == nullptr || m_latestVersion.isEmpty()) {
        return;
    }
    m_settings->setValue(kKeyDismissed, m_latestVersion);
    emit stateChanged(); // re-evaluate `dismissed` in bindings
}

void UpdateManager::setState(State state) {
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}

void UpdateManager::fail(const QString& message) {
    m_lastError = message;
    setState(State::Error);
    emit errorOccurred(message);
}

void UpdateManager::ensureNetwork() {
    if (m_network == nullptr) {
        m_network = new QNetworkAccessManager(this);
    }
    if (m_feed == nullptr) {
        m_feed = new FeedClient(m_network, this);
        connect(m_feed, &FeedClient::fetched, this, &UpdateManager::onFetched);
        connect(m_feed, &FeedClient::notModified, this, &UpdateManager::onNotModified);
        connect(m_feed, &FeedClient::failed, this, [this](const QString& error) { fail(error); });
    }
    if (m_downloader == nullptr) {
        m_downloader = new UpdateDownloader(m_network, this);
        connect(m_downloader, &UpdateDownloader::progress, this, [this](double fraction) {
            m_downloadProgress = fraction;
            emit downloadProgressChanged();
        });
        connect(m_downloader, &UpdateDownloader::finished, this, [this](const QString& path) {
            m_downloadedPath = path;
            setState(State::ReadyToApply);
        });
        connect(m_downloader, &UpdateDownloader::failed, this,
                [this](const QString& error) { fail(error); });
    }
}

void UpdateManager::armPolling() {
    if (m_capability == Capability::None || resolvedFeedUrl().isEmpty()) {
        return; // inert build: no feed, no polling
    }
    if (!autoCheck()) {
        return;
    }
    QTimer::singleShot(kInitialCheckDelayMs, this, [this] {
        if (autoCheck()) {
            check();
        }
    });
    m_pollTimer.start();
}

SelectionCriteria UpdateManager::selectionCriteria() const {
    return {buildArtifactKind(), buildOs(), buildArch(), hostGlibcVersion()};
}

void UpdateManager::check() {
    if (m_capability == Capability::None) {
        return; // fully inert; the platform owns updates
    }
    const QUrl feed = resolvedFeedUrl();
    if (feed.isEmpty()) {
        return;
    }
    if (!FeedClient::isAllowedUrl(feed)) {
        fail(QStringLiteral("feed URL is not an allowed scheme"));
        return;
    }
    ensureNetwork();
    if (m_feed->busy()) {
        return;
    }
    if (m_settings != nullptr) {
        m_settings->setValue(kKeyLastCheck, QDateTime::currentMSecsSinceEpoch());
    }
    setState(State::Checking);
    const QString etag =
        m_settings != nullptr ? m_settings->value(kKeyEtag, QString()).toString() : QString();
    m_feed->fetch(feed, etag);
}

void UpdateManager::onNotModified() {
    // Unchanged manifest since the stored ETag. Re-surface a previously-offered
    // (persisted) version if it is still a non-dismissed upgrade; else up-to-date.
    if (!m_latestVersion.isEmpty() &&
        semver::isUpgrade(m_latestVersion, semver::stripBuildMetadata(currentVersion()))) {
        setState(State::UpdateAvailable);
    } else {
        setState(State::UpToDate);
    }
}

void UpdateManager::onFetched(const QByteArray& manifestBytes, const QByteArray& minisigBytes,
                              const QUrl& resolvedUrl, const QString& newEtag) {
    // 1. Verify the detached signature against the pinned key BEFORE parsing.
    const MinisignVerifier::Result verified = MinisignVerifier::verify(
        manifestBytes, minisigBytes, QStringLiteral(DAEMON_APP_UPDATE_PUBKEY));
    if (!verified.ok) {
        fail(QStringLiteral("signature verification failed: %1").arg(verified.error));
        return;
    }

    // 2. Parse strict schema 1 (product + channel gated).
    const QString channel = QStringLiteral(DAEMON_APP_UPDATE_CHANNEL);
    const ManifestParseResult parsed =
        parseManifest(manifestBytes, QString::fromLatin1(kProduct), channel);
    if (!parsed.ok) {
        fail(parsed.error);
        return;
    }

    // 3. Anti-replay: the trusted comment must carry "daemon <channel> <version>"
    // matching the parsed manifest, so a valid .minisig cannot be replayed onto a
    // different channel's or version's manifest.
    const QString expectedToken =
        QStringLiteral("daemon %1 %2").arg(channel, parsed.manifest.version);
    if (!verified.trustedComment.contains(expectedToken)) {
        fail(QStringLiteral("trusted comment does not match manifest (anti-replay)"));
        return;
    }

    // 4. Store the new ETag now that the manifest is authenticated.
    if (m_settings != nullptr && !newEtag.isEmpty()) {
        m_settings->setValue(kKeyEtag, newEtag);
    }

    evaluateManifest(parsed.manifest, resolvedUrl);
}

void UpdateManager::evaluateManifest(const Manifest& manifest, const QUrl& resolvedUrl) {
    m_manifest = manifest;
    m_resolvedManifestUrl = resolvedUrl;
    m_haveSelected = false;
    m_selected = Artifact{};

    const QString current = semver::stripBuildMetadata(currentVersion());
    if (!semver::isUpgrade(manifest.version, current)) {
        // Monotonicity: never offer an equal/older (possibly replayed) version.
        m_latestVersion.clear();
        m_notesUrl.clear();
        if (m_settings != nullptr) {
            m_settings->setValue(kKeyLatest, QString());
            m_settings->setValue(kKeyNotes, QString());
        }
        m_effectiveCapability = m_capability;
        setState(State::UpToDate);
        return;
    }

    m_latestVersion = manifest.version;
    m_notesUrl = manifest.notesUrl;

    const SelectionResult selection = selectArtifact(manifest, selectionCriteria());
    if (selection.matched) {
        m_selected = selection.artifact;
        m_haveSelected = true;
        // Effective = min(compiled ceiling, feed row's advisory dial).
        m_effectiveCapability =
            minCapability(m_capability, capabilityFromString(m_selected.updateCapability));
    } else {
        // No installable row for this host: Notify-only with the notes link.
        m_effectiveCapability = minCapability(m_capability, Capability::Notify);
    }

    if (m_settings != nullptr) {
        m_settings->setValue(kKeyLatest, m_latestVersion);
        m_settings->setValue(kKeyNotes, m_notesUrl);
    }
    setState(State::UpdateAvailable);
}

void UpdateManager::download() {
    if (m_effectiveCapability < Capability::DownloadAndOpen) {
        fail(QStringLiteral("this build may not download updates"));
        return;
    }
    if (!m_haveSelected) {
        fail(QStringLiteral("no installable artifact to download"));
        return;
    }
    ensureNetwork();
    // Resolve the artifact URL relative to the manifest's own (post-redirect) URL.
    const QUrl artifactUrl = m_resolvedManifestUrl.resolved(QUrl(m_selected.file));
    m_downloadProgress = 0.0;
    emit downloadProgressChanged();
    setState(State::Downloading);
    m_downloader->start(artifactUrl, m_latestVersion, m_selected.file, m_selected.size,
                        m_selected.sha256);
}

void UpdateManager::apply() {
    if (m_state != State::ReadyToApply || m_downloadedPath.isEmpty()) {
        fail(QStringLiteral("no verified update is ready to apply"));
        return;
    }
    if (m_effectiveCapability == Capability::SelfApply) {
        // In-place self-replacement is a later phase (the daemon-updater helper).
        fail(QStringLiteral("self-apply is not implemented yet"));
        return;
    }
    // DownloadAndOpen: hand the verified artifact to the OS. Installers open
    // directly; self-contained/package artifacts reveal their directory instead.
    const QString kind = m_haveSelected ? m_selected.kind : buildArtifactKind();
    const bool openFile = kind == QStringLiteral("nsis") || kind == QStringLiteral("dmg");
    const QUrl target = openFile ? QUrl::fromLocalFile(m_downloadedPath)
                                 : QUrl::fromLocalFile(QFileInfo(m_downloadedPath).absolutePath());
    if (!QDesktopServices::openUrl(target)) {
        fail(QStringLiteral("could not open the downloaded update"));
    }
}

} // namespace update
