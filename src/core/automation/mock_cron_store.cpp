// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "automation/mock_cron_store.h"

#include "appcache/json_cache.h"

namespace automation {
namespace {

const QString kCacheFile = QStringLiteral("cron.json");

QVariantMap mk(const QString& id, const QString& name, const QString& schedule,
               const QString& profile, const QString& prompt, const QString& nextRun,
               const QString& lastRun, bool enabled) {
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("name")] = name;
    m[QStringLiteral("schedule")] = schedule;
    m[QStringLiteral("profile")] = profile;
    m[QStringLiteral("prompt")] = prompt;
    m[QStringLiteral("nextRun")] = nextRun;
    m[QStringLiteral("lastRun")] = lastRun;
    m[QStringLiteral("enabled")] = enabled;
    return m;
}

} // namespace

MockCronStore::MockCronStore(QObject* parent)
    : ICronStore(parent), m_jobs(new uimodels::VariantListModel(this)) {
    m_jobs->upsert(mk(QStringLiteral("c-1"), QStringLiteral("Morning briefing"),
                      QStringLiteral("0 8 * * *"), QStringLiteral("Researcher"),
                      QStringLiteral("Summarize overnight news and my calendar."),
                      QStringLiteral("tomorrow 08:00"), QStringLiteral("today 08:00"), true));
    m_jobs->upsert(mk(QStringLiteral("c-2"), QStringLiteral("Nightly repo digest"),
                      QStringLiteral("0 22 * * 1-5"), QStringLiteral("Coder"),
                      QStringLiteral("Summarize today's commits and open PRs."),
                      QStringLiteral("today 22:00"), QStringLiteral("yesterday 22:00"), true));
    m_jobs->upsert(mk(QStringLiteral("c-3"), QStringLiteral("Weekly cleanup"),
                      QStringLiteral("0 3 * * 0"), QStringLiteral("General Assistant"),
                      QStringLiteral("Archive stale sessions."), QStringLiteral("Sun 03:00"),
                      QStringLiteral("—"), false));
    m_nextId = 4;

    // Restore the last-known snapshot over the seed (no-op on first run).
    const QJsonObject cached = appcache::loadObject(kCacheFile);
    if (cached.contains(QStringLiteral("rows"))) {
        m_jobs->setRows(appcache::rowsFromJson(cached.value(QStringLiteral("rows")).toArray()));
        m_nextId = cached.value(QStringLiteral("nextId")).toInt(m_nextId);
    }
}

void MockCronStore::save() const {
    QJsonObject obj;
    obj[QStringLiteral("rows")] = appcache::rowsToJson(m_jobs->rows());
    obj[QStringLiteral("nextId")] = m_nextId;
    appcache::saveObject(kCacheFile, obj);
}

QObject* MockCronStore::jobs() const {
    return m_jobs;
}

QVariantMap MockCronStore::job(const QString& id) const {
    const int row = m_jobs->indexOfId(id);
    return row >= 0 ? m_jobs->at(row) : QVariantMap{};
}

QString MockCronStore::createJob(const QString& name, const QString& schedule,
                                 const QString& profile) {
    const QString id = QStringLiteral("c-%1").arg(m_nextId++);
    m_jobs->upsert(mk(id, name.isEmpty() ? QStringLiteral("New job") : name,
                      schedule.isEmpty() ? QStringLiteral("0 9 * * *") : schedule,
                      profile.isEmpty() ? QStringLiteral("General Assistant") : profile, QString(),
                      QStringLiteral("pending"), QStringLiteral("—"), true));
    save();
    emit changed();
    return id;
}

void MockCronStore::updateJob(const QString& id, const QVariantMap& fields) {
    const int row = m_jobs->indexOfId(id);
    if (row < 0) {
        return;
    }
    QVariantMap j = m_jobs->at(row);
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        j.insert(it.key(), it.value());
    }
    m_jobs->upsert(j);
    save();
    emit changed();
}

void MockCronStore::setEnabled(const QString& id, bool enabled) {
    updateJob(id, {{QStringLiteral("enabled"), enabled}});
}

void MockCronStore::runNow(const QString& id) {
    updateJob(id, {{QStringLiteral("lastRun"), QStringLiteral("just now")}});
}

void MockCronStore::remove(const QString& id) {
    m_jobs->removeById(id);
    save();
    emit changed();
}

} // namespace automation
