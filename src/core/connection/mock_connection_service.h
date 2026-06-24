#pragma once

#include "connection/iconnection_service.h"

#include <QTimer>

namespace connection {

// Scripted stand-in for the daemon connection. It exercises the liveness state
// machine without pretending that arbitrary targets are live: local targets must
// exist, and remote success is reserved for the explicit mock host. A real
// gateway later replaces this class by driving the same state transitions.
class MockConnectionService : public IConnectionService {
    Q_OBJECT

public:
    explicit MockConnectionService(QObject* parent = nullptr);

    void connectTo(const QString& mode, const QString& target,
                   const QString& token = QString()) override;
    void disconnect() override;
    void testConnection(const QString& mode, const QString& target,
                        const QString& token = QString()) override;

private:
    // Keep mock readiness honest in normal runs. Use local paths that exist or
    // https://mock.local for tests/demos that need a ready remote connection.
    [[nodiscard]] static bool plausible(const QString& mode, const QString& target);

    QTimer m_connectTimer;
    QTimer m_testTimer;
};

} // namespace connection
