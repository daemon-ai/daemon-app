#pragma once

#include "connection/connection_dtos.h"

#include <QObject>
#include <QString>

namespace connection {

// Transport-agnostic connection seam (sibling of IConversationStore /
// IPlatformServices). It owns the active connection to a node, a liveness state
// machine that drives StatusBarModel.gatewayState, and a Test-connection probe.
// The mock cycles believable states now; a daemon adapter (Unix-socket CBOR /
// HTTP JSON) replaces it later by emitting the same state transitions and
// decoding the wire once. UI never sees the codec.
//
// state: "checking" | "connecting" | "ready" | "offline" | "needs setup".
class IConnectionService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY configChanged)
    Q_PROPERTY(QString target READ target NOTIFY configChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY stateChanged)
    Q_PROPERTY(bool testing READ testing NOTIFY testingChanged)

public:
    using QObject::QObject;
    ~IConnectionService() override = default;

    [[nodiscard]] QString state() const { return m_state; }
    [[nodiscard]] QString mode() const { return m_config.mode; }
    [[nodiscard]] QString target() const { return m_config.target; }
    [[nodiscard]] bool ready() const { return m_state == QStringLiteral("ready"); }
    [[nodiscard]] bool testing() const { return m_testing; }
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

signals:
    void stateChanged();
    void configChanged();
    void testingChanged();
    // Probe outcome (Test connection button): ok + a human message.
    void testResult(bool ok, const QString& message);

protected:
    void setState(const QString& s)
    {
        if (m_state != s) {
            m_state = s;
            emit stateChanged();
        }
    }
    void setTesting(bool t)
    {
        if (m_testing != t) {
            m_testing = t;
            emit testingChanged();
        }
    }

    QString m_state = QStringLiteral("checking");
    bool m_testing = false;
    ConnectionConfig m_config;
};

} // namespace connection
