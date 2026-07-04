// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

namespace update {

// Release-feed / auto-update surface (scaffold; design: packaging/UPDATES.md).
//
// The update capability is a per-artifact DIAL FIXED AT PACKAGE TIME: the
// package job compiles the ceiling in via -DDAEMON_APP_UPDATE_CAPABILITY
// (default None, i.e. fully inert — the platform owns updates) and the signed
// feed can only lower it, never raise it. `feedUrl` is likewise baked in via
// -DDAEMON_APP_UPDATE_FEED_URL (default empty: no feed).
//
// Scaffold status: this class exposes the dial + version/feed metadata both
// front ends will bind, and NOTHING else works — check()/download()/apply()
// are honest stubs that report "not implemented". No network, verification,
// download, or apply code exists here yet, and the class is deliberately NOT
// registered with any QML module (UI lands GUI+TUI together per parity rules).
class UpdateManager : public QObject {
    Q_OBJECT
    // Read-only: the compiled-in package-time dial. Runtime can never raise it.
    Q_PROPERTY(Capability capability READ capability CONSTANT)
    // The running build's version string (generated daemon_app_version.h).
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    // Read-only: the compiled-in signed-manifest URL; empty means "no feed".
    Q_PROPERTY(QUrl feedUrl READ feedUrl CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)

public:
    // Strictly ordered ceiling of what this artifact may do about updates:
    // nothing / tell the user / also fetch+verify+hand to the OS / also
    // replace itself. Values mirror the manifest's updateCapability strings.
    enum class Capability { None, Notify, DownloadAndOpen, SelfApply };
    Q_ENUM(Capability)

    // Scaffold lifecycle: Idle until a stub is poked, then Error. The real
    // implementation extends this (Checking/UpdateAvailable/Downloading/...).
    enum class State { Idle, Error };
    Q_ENUM(State)

    explicit UpdateManager(QObject* parent = nullptr);

    [[nodiscard]] Capability capability() const { return m_capability; }
    [[nodiscard]] QString currentVersion() const;
    [[nodiscard]] QUrl feedUrl() const;
    [[nodiscard]] State state() const { return m_state; }
    [[nodiscard]] QString lastError() const { return m_lastError; }

    // Manifest-vocabulary round-trip ("None"/"Notify"/"DownloadAndOpen"/
    // "SelfApply"). Unknown input maps to None: fail closed, never escalate.
    [[nodiscard]] static QString capabilityToString(Capability capability);
    [[nodiscard]] static Capability capabilityFromString(const QString& name);

    // Honest stubs (updates are descoped to design + scaffolding): each
    // transitions to State::Error and emits errorOccurred("... not
    // implemented"). No network or filesystem code may land behind these
    // without the signed-feed verification described in packaging/UPDATES.md.
    Q_INVOKABLE void check();
    Q_INVOKABLE void download();
    Q_INVOKABLE void apply();

signals:
    void stateChanged();
    void errorOccurred(const QString& message);

private:
    void failNotImplemented(const QString& operation);

    Capability m_capability;
    State m_state = State::Idle;
    QString m_lastError;
};

} // namespace update
