#include "fleet/mock_session_roster.h"

namespace fleet {
namespace {

QVariantMap mk(const QString& id, const QString& title, const QString& profile,
               const QString& state, const QString& lifecycle, const QString& lastActivity,
               int tokens, bool rewindable)
{
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("title")] = title;
    m[QStringLiteral("profile")] = profile;
    m[QStringLiteral("state")] = state;
    m[QStringLiteral("lifecycle")] = lifecycle;
    m[QStringLiteral("lastActivity")] = lastActivity;
    m[QStringLiteral("tokens")] = tokens;
    m[QStringLiteral("rewindable")] = rewindable;
    return m;
}

} // namespace

MockSessionRoster::MockSessionRoster(QObject* parent)
    : ISessionRoster(parent)
    , m_sessions(new uimodels::VariantListModel(this))
{
    m_sessions->upsert(mk(QStringLiteral("s-1"), QStringLiteral("Refactor auth module"),
                          QStringLiteral("Coder"), QStringLiteral("active"),
                          QStringLiteral("durable"), QStringLiteral("just now"), 48210, true));
    m_sessions->upsert(mk(QStringLiteral("s-2"), QStringLiteral("Research vector DBs"),
                          QStringLiteral("Researcher"), QStringLiteral("idle"),
                          QStringLiteral("durable"), QStringLiteral("4m ago"), 17650, true));
    m_sessions->upsert(mk(QStringLiteral("s-3"), QStringLiteral("Daily standup summary"),
                          QStringLiteral("General Assistant"), QStringLiteral("suspended"),
                          QStringLiteral("live"), QStringLiteral("1h ago"), 3120, false));
    m_sessions->upsert(mk(QStringLiteral("s-4"), QStringLiteral("Migrate CI pipeline"),
                          QStringLiteral("Coder"), QStringLiteral("active"),
                          QStringLiteral("durable"), QStringLiteral("12m ago"), 62890, true));
    connect(m_sessions, &uimodels::VariantListModel::countChanged, this, &ISessionRoster::changed);
}

QObject* MockSessionRoster::sessions() const { return m_sessions; }
int MockSessionRoster::count() const { return m_sessions->count(); }

void MockSessionRoster::setState(const QString& id, const QString& state)
{
    const int row = m_sessions->indexOfId(id);
    if (row < 0) {
        return;
    }
    QVariantMap s = m_sessions->at(row);
    s[QStringLiteral("state")] = state;
    m_sessions->upsert(s);
    emit changed();
}

void MockSessionRoster::suspend(const QString& id) { setState(id, QStringLiteral("suspended")); }
void MockSessionRoster::resume(const QString& id) { setState(id, QStringLiteral("active")); }

void MockSessionRoster::close(const QString& id)
{
    m_sessions->removeById(id);
    emit changed();
}

} // namespace fleet
