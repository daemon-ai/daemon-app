#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

// Backing model for the footer StatusBar. Holds the daemon-bound status state
// (gateway health, agent counts, busy/turn timing, session timing, context-window
// usage, app version) with placeholder defaults, plus the derived/formatted
// strings the chrome renders. Extracted from StatusBar.qml so the GUI and the TUI
// footer share identical formatting and a single 1s live tick.
//
// GUI-free: Qt6::Qml (QML_ELEMENT) + Qt6::Core (QTimer). Toggles (YOLO/terminal/
// command-center open state) and all layout/popups stay in QML - they are route +
// visual state, not status data.
class StatusBarModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // "ready" | "needs setup" | "checking" | "connecting" | "offline".
    Q_PROPERTY(QString gatewayState READ gatewayState WRITE setGatewayState NOTIFY gatewayStateChanged)
    Q_PROPERTY(int agentsRunning READ agentsRunning WRITE setAgentsRunning NOTIFY agentsRunningChanged)
    Q_PROPERTY(int agentsFailed READ agentsFailed WRITE setAgentsFailed NOTIFY agentsFailedChanged)
    Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged)
    Q_PROPERTY(double turnStartedAt READ turnStartedAt WRITE setTurnStartedAt NOTIFY turnStartedAtChanged)
    Q_PROPERTY(double sessionStartedAt READ sessionStartedAt WRITE setSessionStartedAt NOTIFY sessionStartedAtChanged)
    Q_PROPERTY(int contextUsed READ contextUsed WRITE setContextUsed NOTIFY contextUsedChanged)
    Q_PROPERTY(int contextMax READ contextMax WRITE setContextMax NOTIFY contextMaxChanged)
    Q_PROPERTY(QString appVersion READ appVersion WRITE setAppVersion NOTIFY appVersionChanged)

    // Derived (read-only) gateway helpers.
    Q_PROPERTY(bool gatewayOffline READ gatewayOffline NOTIFY gatewayStateChanged)
    Q_PROPERTY(bool gatewayDegraded READ gatewayDegraded NOTIFY gatewayStateChanged)
    Q_PROPERTY(QString gatewayTone READ gatewayTone NOTIFY gatewayStateChanged)
    Q_PROPERTY(QString agentsDetail READ agentsDetail NOTIFY agentsDetailChanged)
    Q_PROPERTY(int contextPercent READ contextPercent NOTIFY contextChanged)
    Q_PROPERTY(QString contextBar READ contextBar NOTIFY contextChanged)
    Q_PROPERTY(QString contextLabel READ contextLabel NOTIFY contextChanged)
    Q_PROPERTY(QString turnElapsed READ turnElapsed NOTIFY turnElapsedChanged)
    Q_PROPERTY(QString sessionElapsed READ sessionElapsed NOTIFY sessionElapsedChanged)

public:
    explicit StatusBarModel(QObject* parent = nullptr);

    [[nodiscard]] QString gatewayState() const { return m_gatewayState; }
    [[nodiscard]] int agentsRunning() const { return m_agentsRunning; }
    [[nodiscard]] int agentsFailed() const { return m_agentsFailed; }
    [[nodiscard]] bool busy() const { return m_busy; }
    [[nodiscard]] double turnStartedAt() const { return m_turnStartedAt; }
    [[nodiscard]] double sessionStartedAt() const { return m_sessionStartedAt; }
    [[nodiscard]] int contextUsed() const { return m_contextUsed; }
    [[nodiscard]] int contextMax() const { return m_contextMax; }
    [[nodiscard]] QString appVersion() const { return m_appVersion; }

    [[nodiscard]] bool gatewayOffline() const;
    [[nodiscard]] bool gatewayDegraded() const;
    [[nodiscard]] QString gatewayTone() const;
    [[nodiscard]] QString agentsDetail() const;
    [[nodiscard]] int contextPercent() const;
    [[nodiscard]] QString contextBar() const;
    [[nodiscard]] QString contextLabel() const;
    [[nodiscard]] QString turnElapsed() const;
    [[nodiscard]] QString sessionElapsed() const;

    void setGatewayState(const QString& state);
    void setAgentsRunning(int n);
    void setAgentsFailed(int n);
    void setBusy(bool busy);
    void setTurnStartedAt(double ms);
    void setSessionStartedAt(double ms);
    void setContextUsed(int n);
    void setContextMax(int n);
    void setAppVersion(const QString& version);

    Q_INVOKABLE QString formatElapsed(double startMs) const;
    Q_INVOKABLE QString abbrev(int n) const;

signals:
    void gatewayStateChanged();
    void agentsRunningChanged();
    void agentsFailedChanged();
    void busyChanged();
    void turnStartedAtChanged();
    void sessionStartedAtChanged();
    void contextUsedChanged();
    void contextMaxChanged();
    void appVersionChanged();

    void agentsDetailChanged();
    void contextChanged();
    void turnElapsedChanged();
    void sessionElapsedChanged();

private:
    QString m_gatewayState = QStringLiteral("ready");
    int m_agentsRunning = 0;
    int m_agentsFailed = 0;
    bool m_busy = false;
    double m_turnStartedAt = 0;
    double m_sessionStartedAt = 0;
    int m_contextUsed = 12500;
    int m_contextMax = 128000;
    QString m_appVersion = QStringLiteral("v0.1.0");

    // Re-read once per second so the elapsed-time labels stay live.
    double m_nowMs = 0;
    QTimer m_tick;
};
