#include "transcript_exporter.h"

#include "persistence/isession_store.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

QString TranscriptExporter::toJson(QObject* store, int sessionId) const
{
    auto* s = qobject_cast<persistence::ISessionStore*>(store);
    if (s == nullptr) {
        return {};
    }
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), sessionId);
    obj.insert(QStringLiteral("title"), s->title(sessionId));
    obj.insert(QStringLiteral("content"), s->content(sessionId));
    obj.insert(QStringLiteral("exportedAt"),
               QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

bool TranscriptExporter::writeFile(const QUrl& fileUrl, const QString& text) const
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    const QByteArray bytes = text.toUtf8();
    const bool ok = file.write(bytes) == bytes.size();
    file.close();
    return ok;
}

bool TranscriptExporter::exportToPath(persistence::ISessionStore* store, int sessionId,
                                      const QString& filePath) const
{
    if (store == nullptr) {
        return false;
    }
    return writeFile(QUrl::fromLocalFile(filePath), toJson(store, sessionId));
}
