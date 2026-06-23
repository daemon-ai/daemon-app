#pragma once

#include "connection/iconnection_service.h"

#include <QTimer>

namespace connection {

// Scripted stand-in for the daemon connection. connectTo() plays a believable
// checking -> connecting -> ready sequence on a timer (or -> offline for an
// obviously bad target); testConnection() resolves after a short delay. A real
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
    // A target is "good" unless it obviously isn't (empty, or contains "bad"),
    // so the offline / test-failure paths are exercisable from the UI.
    [[nodiscard]] static bool plausible(const QString& mode, const QString& target);

    QTimer m_connectTimer;
    QTimer m_testTimer;
};

} // namespace connection
