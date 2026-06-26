#include "fleet/mock_approvals_inbox.h"

namespace fleet {
namespace {

QVariantMap mk(const QString& id, const QString& session, const QString& tool,
               const QString& command, const QString& risk, const QString& requested) {
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("session")] = session;
    m[QStringLiteral("tool")] = tool;
    m[QStringLiteral("command")] = command;
    m[QStringLiteral("risk")] = risk; // "low" | "medium" | "high"
    m[QStringLiteral("requested")] = requested;
    return m;
}

} // namespace

MockApprovalsInbox::MockApprovalsInbox(QObject* parent)
    : IApprovalsInbox(parent), m_pending(new uimodels::VariantListModel(this)) {
    m_pending->upsert(mk(QStringLiteral("a-1"), QStringLiteral("Refactor auth module"),
                         QStringLiteral("shell"), QStringLiteral("rm -rf build/ && cmake -B build"),
                         QStringLiteral("high"), QStringLiteral("30s ago")));
    m_pending->upsert(mk(QStringLiteral("a-2"), QStringLiteral("Migrate CI pipeline"),
                         QStringLiteral("write"), QStringLiteral(".github/workflows/ci.yml"),
                         QStringLiteral("medium"), QStringLiteral("2m ago")));
    m_pending->upsert(mk(QStringLiteral("a-3"), QStringLiteral("Research vector DBs"),
                         QStringLiteral("browse"), QStringLiteral("https://pgvector.dev"),
                         QStringLiteral("low"), QStringLiteral("5m ago")));
    connect(m_pending, &uimodels::VariantListModel::countChanged, this, &IApprovalsInbox::changed);
}

QObject* MockApprovalsInbox::pending() const {
    return m_pending;
}
int MockApprovalsInbox::count() const {
    return m_pending->count();
}

void MockApprovalsInbox::approve(const QString& id) {
    m_pending->removeById(id);
}
void MockApprovalsInbox::deny(const QString& id) {
    m_pending->removeById(id);
}

} // namespace fleet
