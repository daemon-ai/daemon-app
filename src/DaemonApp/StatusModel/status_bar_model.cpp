#include "status_bar_model.h"

#include <cmath>
#include <QDateTime>
#include <QVariantMap>

StatusBarModel::StatusBarModel(QObject* parent) : QObject(parent) {
    // Empty by default: the connection/daemon adapter owns live gateway facts.
    // Mock/demo providers may still set these through the public setters.
    m_gatewayConnectionText = tr("No daemon connection");
    m_gatewayLog = {};
    m_gatewayPlatforms = {};

    m_nowMs = static_cast<double>(QDateTime::currentMSecsSinceEpoch());
    // Session timer starts at launch (mirrors StatusBar.qml's
    // Component.onCompleted: sessionStartedAt = Date.now()).
    m_sessionStartedAt = m_nowMs;

    m_tick.setInterval(1000);
    connect(&m_tick, &QTimer::timeout, this, [this] {
        m_nowMs = static_cast<double>(QDateTime::currentMSecsSinceEpoch());
        emit turnElapsedChanged();
        emit sessionElapsedChanged();
    });
    m_tick.start();
}

bool StatusBarModel::gatewayOffline() const {
    return m_gatewayState == QStringLiteral("offline");
}

bool StatusBarModel::gatewayDegraded() const {
    return m_gatewayState == QStringLiteral("needs setup") ||
           m_gatewayState == QStringLiteral("connecting") ||
           m_gatewayState == QStringLiteral("checking");
}

QString StatusBarModel::gatewayTone() const {
    if (gatewayOffline()) {
        return QStringLiteral("danger");
    }
    if (gatewayDegraded()) {
        return QStringLiteral("warning");
    }
    return QStringLiteral("default");
}

QString StatusBarModel::agentsDetail() const {
    if (m_agentsFailed > 0) {
        return tr("%n failed", nullptr, m_agentsFailed);
    }
    if (m_agentsRunning > 0) {
        return tr("%n running", nullptr, m_agentsRunning);
    }
    return QString();
}

int StatusBarModel::contextPercent() const {
    return m_contextMax > 0 ? static_cast<int>(std::lround(
                                  (static_cast<double>(m_contextUsed) / m_contextMax) * 100.0))
                            : 0;
}

QString StatusBarModel::contextBar() const {
    const int percent = contextPercent();
    int filled = static_cast<int>(std::lround(percent / 10.0));
    filled = qBound(0, filled, 10);
    QString bar;
    for (int i = 0; i < 10; ++i) {
        bar += (i < filled ? QStringLiteral("\u2588") : QStringLiteral("\u2591"));
    }
    return QStringLiteral("[") + bar + QStringLiteral("] ") + QString::number(percent) +
           QStringLiteral("%");
}

QString StatusBarModel::contextLabel() const {
    if (m_contextMax > 0) {
        return abbrev(m_contextUsed) + QStringLiteral("/") + abbrev(m_contextMax);
    }
    return abbrev(m_contextUsed) + tr(" tok");
}

QString StatusBarModel::costLabel() const {
    return QStringLiteral("$") + QString::number(m_usdCost, 'f', m_usdCost >= 1.0 ? 2 : 3);
}

QString StatusBarModel::rateLabel() const {
    if (m_rateLimit <= 0) {
        return QString();
    }
    return QString::number(m_rateRemaining) + QStringLiteral("/") + QString::number(m_rateLimit);
}

QString StatusBarModel::turnElapsed() const {
    return formatElapsed(m_turnStartedAt);
}

QString StatusBarModel::sessionElapsed() const {
    return formatElapsed(m_sessionStartedAt);
}

void StatusBarModel::setGatewayState(const QString& state) {
    if (m_gatewayState == state) {
        return;
    }
    m_gatewayState = state;
    emit gatewayStateChanged();
}

void StatusBarModel::setAgentsRunning(int n) {
    if (m_agentsRunning == n) {
        return;
    }
    m_agentsRunning = n;
    emit agentsRunningChanged();
    emit agentsDetailChanged();
}

void StatusBarModel::setAgentsFailed(int n) {
    if (m_agentsFailed == n) {
        return;
    }
    m_agentsFailed = n;
    emit agentsFailedChanged();
    emit agentsDetailChanged();
}

void StatusBarModel::setBusy(bool busy) {
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void StatusBarModel::setTurnStartedAt(double ms) {
    if (qFuzzyCompare(m_turnStartedAt, ms)) {
        return;
    }
    m_turnStartedAt = ms;
    emit turnStartedAtChanged();
    emit turnElapsedChanged();
}

void StatusBarModel::setSessionStartedAt(double ms) {
    if (qFuzzyCompare(m_sessionStartedAt, ms)) {
        return;
    }
    m_sessionStartedAt = ms;
    emit sessionStartedAtChanged();
    emit sessionElapsedChanged();
}

void StatusBarModel::setContextUsed(int n) {
    if (m_contextUsed == n) {
        return;
    }
    m_contextUsed = n;
    emit contextUsedChanged();
    emit contextChanged();
}

void StatusBarModel::setContextMax(int n) {
    if (m_contextMax == n) {
        return;
    }
    m_contextMax = n;
    emit contextMaxChanged();
    emit contextChanged();
}

void StatusBarModel::setTokensIn(int n) {
    if (m_tokensIn == n) {
        return;
    }
    m_tokensIn = n;
    emit usageChanged();
}

void StatusBarModel::setTokensOut(int n) {
    if (m_tokensOut == n) {
        return;
    }
    m_tokensOut = n;
    emit usageChanged();
}

void StatusBarModel::setUsdCost(double cost) {
    if (qFuzzyCompare(m_usdCost, cost)) {
        return;
    }
    m_usdCost = cost;
    emit usageChanged();
}

void StatusBarModel::setRateRemaining(int n) {
    if (m_rateRemaining == n) {
        return;
    }
    m_rateRemaining = n;
    emit rateChanged();
}

void StatusBarModel::setRateLimit(int n) {
    if (m_rateLimit == n) {
        return;
    }
    m_rateLimit = n;
    emit rateChanged();
}

void StatusBarModel::applyTurnEvents(const QVariantList& events) {
    bool usage = false;
    bool rate = false;
    for (const QVariant& e : events) {
        const QVariantMap event = e.toMap();
        const QString type = event.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("usage")) {
            m_tokensIn += event.value(QStringLiteral("tokensIn")).toInt();
            m_tokensOut += event.value(QStringLiteral("tokensOut")).toInt();
            m_usdCost += event.value(QStringLiteral("costUsd")).toDouble();
            usage = true;
        } else if (type == QStringLiteral("context")) {
            setContextUsed(event.value(QStringLiteral("used")).toInt());
            if (event.contains(QStringLiteral("max"))) {
                setContextMax(event.value(QStringLiteral("max")).toInt());
            }
        } else if (type == QStringLiteral("rateLimit")) {
            m_rateRemaining = event.value(QStringLiteral("remaining")).toInt();
            m_rateLimit = event.value(QStringLiteral("limit")).toInt();
            rate = true;
        }
    }
    if (usage) {
        emit usageChanged();
    }
    if (rate) {
        emit rateChanged();
    }
}

void StatusBarModel::resetUsage() {
    m_tokensIn = 0;
    m_tokensOut = 0;
    m_usdCost = 0.0;
    emit usageChanged();
}

void StatusBarModel::setAppVersion(const QString& version) {
    if (m_appVersion == version) {
        return;
    }
    m_appVersion = version;
    emit appVersionChanged();
}

void StatusBarModel::setGatewayConnectionText(const QString& text) {
    if (m_gatewayConnectionText == text) {
        return;
    }
    m_gatewayConnectionText = text;
    emit gatewayInfoChanged();
}

void StatusBarModel::setGatewayLog(const QStringList& log) {
    if (m_gatewayLog == log) {
        return;
    }
    m_gatewayLog = log;
    emit gatewayInfoChanged();
}

void StatusBarModel::setGatewayPlatforms(const QVariantList& platforms) {
    if (m_gatewayPlatforms == platforms) {
        return;
    }
    m_gatewayPlatforms = platforms;
    emit gatewayInfoChanged();
}

QString StatusBarModel::formatElapsed(double startMs) const {
    if (startMs == 0) {
        return QStringLiteral("0:00");
    }
    const int total = qMax(0, static_cast<int>(std::floor((m_nowMs - startMs) / 1000.0)));
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    const QString ss = s < 10 ? (QStringLiteral("0") + QString::number(s)) : QString::number(s);
    if (h > 0) {
        const QString mm = m < 10 ? (QStringLiteral("0") + QString::number(m)) : QString::number(m);
        return QString::number(h) + QStringLiteral(":") + mm + QStringLiteral(":") + ss;
    }
    return QString::number(m) + QStringLiteral(":") + ss;
}

QString StatusBarModel::abbrev(int n) const {
    const auto trim = [](double value, const QString& suffix) {
        QString text = QString::number(value, 'f', 1);
        if (text.endsWith(QStringLiteral(".0"))) {
            text.chop(2);
        }
        return text + suffix;
    };
    if (n >= 1000000) {
        return trim(n / 1000000.0, QStringLiteral("M"));
    }
    if (n >= 1000) {
        return trim(n / 1000.0, QStringLiteral("k"));
    }
    return QString::number(n);
}
