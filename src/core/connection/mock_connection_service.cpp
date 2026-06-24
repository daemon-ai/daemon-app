#include "connection/mock_connection_service.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

namespace connection {

namespace {
QString expandHome(QString path)
{
    path = path.trimmed();
    if (path == QStringLiteral("~")) {
        return QDir::homePath();
    }
    if (path.startsWith(QStringLiteral("~/"))) {
        return QDir::homePath() + path.mid(1);
    }
    return path;
}
} // namespace

MockConnectionService::MockConnectionService(QObject* parent)
    : IConnectionService(parent)
{
    m_connectTimer.setSingleShot(true);
    m_testTimer.setSingleShot(true);
}

bool MockConnectionService::plausible(const QString& mode, const QString& target)
{
    const QString trimmed = target.trimmed();
    if (mode == QStringLiteral("embedded")) {
        return false; // gated until full-node FFI lands
    }
    if (trimmed.isEmpty()) {
        return false;
    }
    if (mode == QStringLiteral("local")) {
        return QFileInfo::exists(expandHome(trimmed));
    }
    if (mode == QStringLiteral("remote")) {
        const QUrl url(trimmed);
        return url.isValid() && (url.scheme() == QStringLiteral("http")
                                 || url.scheme() == QStringLiteral("https"))
            && url.host() == QStringLiteral("mock.local");
    }
    return mode == QStringLiteral("mock") && trimmed == QStringLiteral("ready");
}

void MockConnectionService::connectTo(const QString& mode, const QString& target,
                                      const QString& token)
{
    m_config.mode = mode;
    m_config.target = target;
    m_config.token = token;
    emit configChanged();

    m_connectTimer.stop();
    setState(QStringLiteral("checking"));

    const bool ok = plausible(mode, target);

    // checking -> connecting (250ms) -> ready/offline (+450ms).
    QTimer::singleShot(250, this, [this] { setState(QStringLiteral("connecting")); });
    m_connectTimer.disconnect();
    connect(&m_connectTimer, &QTimer::timeout, this, [this, ok] {
        setState(ok ? QStringLiteral("ready") : QStringLiteral("offline"));
    });
    m_connectTimer.start(700);
}

void MockConnectionService::disconnect()
{
    m_connectTimer.stop();
    setState(QStringLiteral("offline"));
}

void MockConnectionService::testConnection(const QString& mode, const QString& target,
                                           const QString& token)
{
    Q_UNUSED(token)
    setTesting(true);
    m_testTimer.stop();
    m_testTimer.disconnect();
    const bool ok = plausible(mode, target);
    connect(&m_testTimer, &QTimer::timeout, this, [this, ok, target] {
        setTesting(false);
        emit testResult(ok, ok ? QStringLiteral("Connected to %1").arg(target)
                                : QStringLiteral("Could not reach %1").arg(target));
    });
    m_testTimer.start(600);
}

} // namespace connection
