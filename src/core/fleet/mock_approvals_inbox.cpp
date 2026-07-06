// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "fleet/mock_approvals_inbox.h"

namespace fleet {
namespace {

QVariantMap mk(const QString& id, const QString& session, const QString& tool,
               const QString& command, const QString& risk, const QString& requested,
               bool canAllowPermanent) {
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("session")] = session;
    m[QStringLiteral("tool")] = tool;
    m[QStringLiteral("command")] = command;
    m[QStringLiteral("risk")] = risk; // "low" | "medium" | "high"
    m[QStringLiteral("requested")] = requested;
    // Whether the node could remember this approval permanently (fingerprinted). Mixed across the
    // mock rows so the offer-gating is exercised in mock mode.
    m[QStringLiteral("canAllowPermanent")] = canAllowPermanent;
    return m;
}

} // namespace

MockApprovalsInbox::MockApprovalsInbox(QObject* parent)
    : IApprovalsInbox(parent), m_pending(new uimodels::VariantListModel(this)) {
    // a-1 is a fingerprinted command exec (offers "allow permanently"); a-2/a-3 are not (a file
    // write and a browse have no command fingerprint), so their offer stays hidden.
    m_pending->upsert(mk(QStringLiteral("a-1"), QStringLiteral("Refactor auth module"),
                         QStringLiteral("shell"), QStringLiteral("rm -rf build/ && cmake -B build"),
                         QStringLiteral("high"), QStringLiteral("30s ago"),
                         /*canAllowPermanent=*/true));
    m_pending->upsert(mk(QStringLiteral("a-2"), QStringLiteral("Migrate CI pipeline"),
                         QStringLiteral("write"), QStringLiteral(".github/workflows/ci.yml"),
                         QStringLiteral("medium"), QStringLiteral("2m ago"),
                         /*canAllowPermanent=*/false));
    m_pending->upsert(mk(QStringLiteral("a-3"), QStringLiteral("Research vector DBs"),
                         QStringLiteral("browse"), QStringLiteral("https://pgvector.dev"),
                         QStringLiteral("low"), QStringLiteral("5m ago"),
                         /*canAllowPermanent=*/false));
    connect(m_pending, &uimodels::VariantListModel::countChanged, this, &IApprovalsInbox::changed);
}

QObject* MockApprovalsInbox::pending() const {
    return m_pending;
}
int MockApprovalsInbox::count() const {
    return m_pending->count();
}

void MockApprovalsInbox::approve(const QString& id, bool /*allowPermanent*/) {
    // The mock has no node to remember a permanent decision; both approve and approve-permanently
    // simply resolve the row. The flag is accepted for seam parity with the daemon-backed inbox.
    m_pending->removeById(id);
}
void MockApprovalsInbox::deny(const QString& id, const QString& reason) {
    // [wave2:app-approvals-safety] D3: the mock has no wire; the reason is accepted for seam parity
    // with the daemon-backed inbox and simply drops the row.
    Q_UNUSED(reason)
    m_pending->removeById(id);
}

} // namespace fleet
