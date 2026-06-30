// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "connection/connection_dtos.h"

#include <QObject>
#include <QString>

namespace connection {

// Transport-agnostic connection seam (sibling of ISessionStore /
// IPlatformServices). It owns the active connection to a node, a liveness state
// machine that drives StatusBarModel.gatewayState, and a Test-connection probe.
// The mock cycles believable states now; a daemon adapter (Unix-socket CBOR /
// HTTP JSON) replaces it later by emitting the same state transitions and
// decoding the wire once. UI never sees the codec.
//
// state: "checking" | "connecting" | "authenticating" | "ready" | "offline" | "needs setup".
// "authenticating" means the node requires SASL credentials the client does not yet have (or a
// login attempt failed); the first-run/login UI collects them and calls login().
class IConnectionService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY configChanged)
    Q_PROPERTY(QString target READ target NOTIFY configChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY stateChanged)
    Q_PROPERTY(bool testing READ testing NOTIFY testingChanged)
    // A human-readable detail for the current state (e.g. an actionable reason a local connect
    // failed). Empty when there is nothing specific to say; the UI falls back to generic copy.
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY stateChanged)

public:
    using QObject::QObject;
    ~IConnectionService() override = default;

    [[nodiscard]] QString state() const { return m_state; }
    [[nodiscard]] QString mode() const { return m_config.mode; }
    [[nodiscard]] QString target() const { return m_config.target; }
    [[nodiscard]] bool ready() const { return m_state == QStringLiteral("ready"); }
    [[nodiscard]] bool testing() const { return m_testing; }
    [[nodiscard]] QString statusMessage() const { return m_statusMessage; }
    [[nodiscard]] ConnectionConfig config() const { return m_config; }

    // Open (or switch) the active connection. Drives the liveness state machine
    // (checking -> connecting -> ready / offline).
    Q_INVOKABLE virtual void connectTo(const QString& mode, const QString& target,
                                       const QString& token = QString()) = 0;
    // Drop the active connection (state -> offline). Cached data stays readable.
    Q_INVOKABLE virtual void disconnect() = 0;
    // Probe a candidate target without committing; result via testResult.
    Q_INVOKABLE virtual void testConnection(const QString& mode, const QString& target,
                                            const QString& token = QString()) = 0;
    // Submit interactive credentials for a node that requires authentication (state
    // "authenticating"). Default no-op so mock/embedded seams need not implement it.
    Q_INVOKABLE virtual void login(const QString& username, const QString& password) {
        Q_UNUSED(username)
        Q_UNUSED(password)
    }
    // Clear the persisted session token for the active/last target (logout). Default no-op.
    Q_INVOKABLE virtual void logout() {}

signals:
    void stateChanged();
    void configChanged();
    void testingChanged();
    // Probe outcome (Test connection button): ok + a human message.
    void testResult(bool ok, const QString& message);
    // The connected node requires interactive credentials we don't have (show the login form).
    void authRequired();
    // A login attempt failed (wrong password / disabled account). `reason` is coarse by design.
    void authFailed(const QString& reason);
    // SASL authentication succeeded; the bound principal is now available downstream.
    void authenticated();

protected:
    void setState(const QString& s) {
        if (m_state != s) {
            m_state = s;
            emit stateChanged();
        }
    }
    void setTesting(bool t) {
        if (m_testing != t) {
            m_testing = t;
            emit testingChanged();
        }
    }
    void setStatusMessage(const QString& message) {
        if (m_statusMessage != message) {
            m_statusMessage = message;
            emit stateChanged();
        }
    }

    QString m_state = QStringLiteral("checking");
    bool m_testing = false;
    QString m_statusMessage;
    ConnectionConfig m_config;
};

} // namespace connection
