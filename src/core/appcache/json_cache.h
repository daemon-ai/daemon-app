#pragma once

// Header-only last-known on-disk cache for the daemon-config-domain mocks
// (profiles / accounts / models / cron / daemon config). Each mock loads its
// snapshot on construct and writes it back on every mutation, so the GUI/TUI
// keep their seeded state across restarts while there is no daemon to be the
// source of truth. JSON under the app data dir keeps the files human-inspectable
// and matches the location SqliteSessionStore already uses.
//
// Header-only (inline, no .cpp) so the five independent mock libraries can each
// include it without a shared link dependency. When the daemon becomes
// authoritative these caches become redundant (the audit's "read-only/last-known
// when offline" layer) and the mocks fall away wholesale.

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QStandardPaths>
#include <QString>
#include <QVariantMap>

namespace appcache {

// The writable app-data directory (created if missing), or the temp dir as a
// last resort. Same root the durable conversation store writes into.
inline QString dir()
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (d.isEmpty()) {
        d = QDir::tempPath();
    }
    QDir().mkpath(d);
    return d;
}

inline QString path(const QString& file)
{
    return dir() + QLatin1Char('/') + file;
}

// Read a cached JSON object (empty if the file is missing / unreadable / not an
// object), so a first run with no cache transparently falls back to the seed.
inline QJsonObject loadObject(const QString& file)
{
    QFile f(path(file));
    if (!f.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(f.readAll()).object();
}

inline void saveObject(const QString& file, const QJsonObject& obj)
{
    QFile f(path(file));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

// QVariantMap rows <-> a JSON array, the shape every VariantListModel-backed mock
// (profiles / accounts / models / cron) persists. String-list fields survive the
// round-trip via QVariant::toStringList() on the QVariantList JSON produces.
inline QJsonArray rowsToJson(const QList<QVariantMap>& rows)
{
    QJsonArray arr;
    for (const QVariantMap& r : rows) {
        arr.append(QJsonObject::fromVariantMap(r));
    }
    return arr;
}

inline QList<QVariantMap> rowsFromJson(const QJsonArray& arr)
{
    QList<QVariantMap> rows;
    rows.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        rows.append(v.toObject().toVariantMap());
    }
    return rows;
}

} // namespace appcache
