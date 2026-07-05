// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "update/update_manifest.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
QT_END_NAMESPACE

namespace settings {
class ISettingsStore;
}

namespace update {

class FeedClient;
class UpdateDownloader;

// Release-feed / auto-update surface (design: packaging/UPDATES.md).
//
// The update capability is a per-artifact DIAL FIXED AT PACKAGE TIME: the package
// job compiles the ceiling in via -DDAEMON_APP_UPDATE_CAPABILITY (default None,
// fully inert) and the signed feed can only LOWER it, never raise it. The feed
// URL, the pinned minisign public key, the artifact kind, and the channel are
// likewise baked in.
//
// Flow (Notify + DownloadAndOpen): fetch manifest.json + .minisig -> verify the
// signature against the pinned key -> parse schema 1 -> SemVer monotonic gate
// (offer only version > current) -> select the artifact for this build -> Notify.
// download() then fetches + sha256-gates the artifact; apply() reveals/opens it
// (DownloadAndOpen). SelfApply is a later phase (an honest not-implemented error).
//
// Both front ends bind this as the `Update` context property / RootWidget member.
class UpdateManager : public QObject {
    Q_OBJECT
    // Read-only: the compiled-in package-time ceiling. Runtime can never raise it.
    Q_PROPERTY(Capability capability READ capability CONSTANT)
    // min(compiled, feed row) once a manifest has been evaluated; the UI gates the
    // Download/Open actions on this. Defaults to the compiled ceiling.
    Q_PROPERTY(Capability effectiveCapability READ effectiveCapability NOTIFY stateChanged)
    // The running build's version string (generated daemon_app_version.h).
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    // Read-only: the effective signed-manifest URL (compiled, or the test-only env
    // override when the build is not inert); empty means "no feed".
    Q_PROPERTY(QUrl feedUrl READ feedUrl CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)
    // The newest offered version (empty unless state is UpdateAvailable/beyond).
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY stateChanged)
    // The release-notes URL for the offered version (may be empty).
    Q_PROPERTY(QString notesUrl READ notesUrl NOTIFY stateChanged)
    // Download progress 0..1 while state is Downloading.
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    // True when the offered version was dismissed by the user (don't re-nag).
    Q_PROPERTY(bool dismissed READ dismissed NOTIFY stateChanged)
    // Auto-check preference (persisted; default on). Manual check() ignores it.
    Q_PROPERTY(bool autoCheck READ autoCheck WRITE setAutoCheck NOTIFY autoCheckChanged)

public:
    // Strictly ordered ceiling of what this artifact may do about updates:
    // nothing / tell the user / also fetch+verify+hand to the OS / also replace
    // itself. Values mirror the manifest's updateCapability strings.
    enum class Capability { None, Notify, DownloadAndOpen, SelfApply };
    Q_ENUM(Capability)

    // The updater lifecycle: idle at rest, then a check cycle that lands on
    // UpToDate / UpdateAvailable / Error, and (DownloadAndOpen) the download ->
    // ReadyToApply path.
    enum class State {
        Idle,
        Checking,
        UpToDate,
        UpdateAvailable,
        Downloading,
        ReadyToApply,
        Error
    };
    Q_ENUM(State)

    explicit UpdateManager(QObject* parent = nullptr);
    ~UpdateManager() override;

    // Bind the client-local settings store (poll bookkeeping: lastCheck, etag,
    // dismissed version, auto-check). Arms polling + runs staging hygiene. Safe to
    // call once, from the service graph factory.
    void setSettings(settings::ISettingsStore* settings);

    [[nodiscard]] Capability capability() const { return m_capability; }
    [[nodiscard]] Capability effectiveCapability() const { return m_effectiveCapability; }
    [[nodiscard]] QString currentVersion() const;
    [[nodiscard]] QUrl feedUrl() const;
    [[nodiscard]] State state() const { return m_state; }
    [[nodiscard]] QString lastError() const { return m_lastError; }
    [[nodiscard]] QString latestVersion() const { return m_latestVersion; }
    [[nodiscard]] QString notesUrl() const { return m_notesUrl; }
    [[nodiscard]] double downloadProgress() const { return m_downloadProgress; }
    [[nodiscard]] bool dismissed() const;
    [[nodiscard]] bool autoCheck() const;
    void setAutoCheck(bool on);

    // Manifest-vocabulary round-trip ("None"/"Notify"/"DownloadAndOpen"/
    // "SelfApply"). Unknown input maps to None: fail closed, never escalate.
    [[nodiscard]] static QString capabilityToString(Capability capability);
    [[nodiscard]] static Capability capabilityFromString(const QString& name);

    // Manual check: fetch + verify + evaluate the feed now. No-op on an inert
    // (None / no-feed) build.
    Q_INVOKABLE void check();
    // Download the offered artifact (requires effectiveCapability >=
    // DownloadAndOpen and a selected artifact). Verifies size + sha256 before
    // exposing the file.
    Q_INVOKABLE void download();
    // Hand the verified artifact to the OS (DownloadAndOpen): open the installer
    // (.exe/.dmg) or reveal the containing directory (AppImage/deb/rpm/tar).
    // SelfApply is a later phase and reports not-implemented here.
    Q_INVOKABLE void apply();
    // Remember the offered version as dismissed so the UI stops nagging for it.
    Q_INVOKABLE void dismiss();

signals:
    void stateChanged();
    void errorOccurred(const QString& message);
    void downloadProgressChanged();
    void autoCheckChanged();

private:
    void setState(State state);
    void fail(const QString& message);
    [[nodiscard]] QUrl resolvedFeedUrl() const;
    [[nodiscard]] SelectionCriteria selectionCriteria() const;
    void ensureNetwork();
    void armPolling();
    void onFetched(const QByteArray& manifestBytes, const QByteArray& minisigBytes,
                   const QUrl& resolvedUrl, const QString& newEtag);
    void onNotModified();
    void evaluateManifest(const Manifest& manifest, const QUrl& resolvedUrl);

    Capability m_capability;
    Capability m_effectiveCapability;
    State m_state = State::Idle;
    QString m_lastError;
    QString m_latestVersion;
    QString m_notesUrl;
    double m_downloadProgress = 0.0;

    settings::ISettingsStore* m_settings = nullptr;
    QNetworkAccessManager* m_network = nullptr;
    FeedClient* m_feed = nullptr;
    UpdateDownloader* m_downloader = nullptr;
    QTimer m_pollTimer;

    // The evaluated manifest state carried between check -> download -> apply.
    Manifest m_manifest;
    Artifact m_selected;
    bool m_haveSelected = false;
    QUrl m_resolvedManifestUrl;
    QString m_downloadedPath;
};

} // namespace update
