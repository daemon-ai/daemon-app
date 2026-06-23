#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
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
    // Cumulative token usage + spend for the session (fed by the turn's usage
    // events). The daemon will later emit the same usage deltas.
    Q_PROPERTY(int tokensIn READ tokensIn WRITE setTokensIn NOTIFY usageChanged)
    Q_PROPERTY(int tokensOut READ tokensOut WRITE setTokensOut NOTIFY usageChanged)
    Q_PROPERTY(double usdCost READ usdCost WRITE setUsdCost NOTIFY usageChanged)
    // Provider rate-limit window (remaining/limit for the current window).
    Q_PROPERTY(int rateRemaining READ rateRemaining WRITE setRateRemaining NOTIFY rateChanged)
    Q_PROPERTY(int rateLimit READ rateLimit WRITE setRateLimit NOTIFY rateChanged)
    Q_PROPERTY(QString appVersion READ appVersion WRITE setAppVersion NOTIFY appVersionChanged)

    // Derived (read-only) gateway helpers.
    Q_PROPERTY(bool gatewayOffline READ gatewayOffline NOTIFY gatewayStateChanged)
    Q_PROPERTY(bool gatewayDegraded READ gatewayDegraded NOTIFY gatewayStateChanged)
    Q_PROPERTY(QString gatewayTone READ gatewayTone NOTIFY gatewayStateChanged)

    // Gateway dropdown content (placeholder until wired to the daemon). Lives here
    // so the GUI GatewayMenu and a future TUI gateway view share one source; the
    // GatewayMenu no longer re-derives degraded/offline locally. `gatewayPlatforms`
    // is a list of { name: QString, online: bool } maps.
    Q_PROPERTY(QString gatewayConnectionText READ gatewayConnectionText WRITE
                   setGatewayConnectionText NOTIFY gatewayInfoChanged)
    Q_PROPERTY(QStringList gatewayLog READ gatewayLog WRITE setGatewayLog NOTIFY gatewayInfoChanged)
    Q_PROPERTY(QVariantList gatewayPlatforms READ gatewayPlatforms WRITE setGatewayPlatforms NOTIFY
                   gatewayInfoChanged)
    Q_PROPERTY(QString agentsDetail READ agentsDetail NOTIFY agentsDetailChanged)
    Q_PROPERTY(int contextPercent READ contextPercent NOTIFY contextChanged)
    Q_PROPERTY(QString contextBar READ contextBar NOTIFY contextChanged)
    Q_PROPERTY(QString contextLabel READ contextLabel NOTIFY contextChanged)
    Q_PROPERTY(QString costLabel READ costLabel NOTIFY usageChanged)
    Q_PROPERTY(QString rateLabel READ rateLabel NOTIFY rateChanged)
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
    [[nodiscard]] int tokensIn() const { return m_tokensIn; }
    [[nodiscard]] int tokensOut() const { return m_tokensOut; }
    [[nodiscard]] double usdCost() const { return m_usdCost; }
    [[nodiscard]] int rateRemaining() const { return m_rateRemaining; }
    [[nodiscard]] int rateLimit() const { return m_rateLimit; }
    [[nodiscard]] QString appVersion() const { return m_appVersion; }

    [[nodiscard]] bool gatewayOffline() const;
    [[nodiscard]] bool gatewayDegraded() const;
    [[nodiscard]] QString gatewayTone() const;
    [[nodiscard]] QString gatewayConnectionText() const { return m_gatewayConnectionText; }
    [[nodiscard]] QStringList gatewayLog() const { return m_gatewayLog; }
    [[nodiscard]] QVariantList gatewayPlatforms() const { return m_gatewayPlatforms; }
    [[nodiscard]] QString agentsDetail() const;
    [[nodiscard]] int contextPercent() const;
    [[nodiscard]] QString contextBar() const;
    [[nodiscard]] QString contextLabel() const;
    [[nodiscard]] QString costLabel() const;
    [[nodiscard]] QString rateLabel() const;
    [[nodiscard]] QString turnElapsed() const;
    [[nodiscard]] QString sessionElapsed() const;

    void setGatewayState(const QString& state);
    void setAgentsRunning(int n);
    void setAgentsFailed(int n);
    Q_INVOKABLE void setBusy(bool busy);
    Q_INVOKABLE void setTurnStartedAt(double ms);
    Q_INVOKABLE void setSessionStartedAt(double ms);
    Q_INVOKABLE void setContextUsed(int n);
    void setContextMax(int n);
    void setTokensIn(int n);
    void setTokensOut(int n);
    void setUsdCost(double cost);
    void setRateRemaining(int n);
    void setRateLimit(int n);
    void setAppVersion(const QString& version);
    void setGatewayConnectionText(const QString& text);
    void setGatewayLog(const QStringList& log);
    void setGatewayPlatforms(const QVariantList& platforms);

    Q_INVOKABLE QString formatElapsed(double startMs) const;
    Q_INVOKABLE QString abbrev(int n) const;
    // Apply a turn's status-only events (usage/context/rateLimit) - usage deltas
    // accumulate, context + rate are absolute. The same shapes the daemon emits.
    Q_INVOKABLE void applyTurnEvents(const QVariantList& events);
    // Zero the per-session usage + cost counters (e.g. a /compress or new session).
    Q_INVOKABLE void resetUsage();

signals:
    void gatewayStateChanged();
    void gatewayInfoChanged();
    void agentsRunningChanged();
    void agentsFailedChanged();
    void busyChanged();
    void turnStartedAtChanged();
    void sessionStartedAtChanged();
    void contextUsedChanged();
    void contextMaxChanged();
    void usageChanged();
    void rateChanged();
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
    int m_tokensIn = 0;
    int m_tokensOut = 0;
    double m_usdCost = 0.0;
    int m_rateRemaining = 0;
    int m_rateLimit = 0;
    QString m_appVersion = QStringLiteral("v0.1.0");

    // Gateway dropdown content (placeholder, seeded in the constructor to match the
    // values the QML GatewayMenu previously hardcoded).
    QString m_gatewayConnectionText;
    QStringList m_gatewayLog;
    QVariantList m_gatewayPlatforms;

    // Re-read once per second so the elapsed-time labels stay live.
    double m_nowMs = 0;
    QTimer m_tick;
};
