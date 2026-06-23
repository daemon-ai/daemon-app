#include "fleet/mock_fleet_tree.h"

namespace fleet {
namespace {

QVariantMap mk(int depth, const QString& id, const QString& name, const QString& kind,
               const QString& status, const QString& model)
{
    QVariantMap m;
    m[QStringLiteral("depth")] = depth;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("name")] = name;
    m[QStringLiteral("kind")] = kind;     // "orchestrator" | "worker"
    m[QStringLiteral("status")] = status; // "running" | "paused" | "idle"
    m[QStringLiteral("model")] = model;
    return m;
}

} // namespace

MockFleetTree::MockFleetTree(QObject* parent)
    : IFleetTree(parent)
    , m_nodes(new uimodels::VariantListModel(this))
{
    m_nodes->upsert(mk(0, QStringLiteral("n-root"), QStringLiteral("Orchestrator"),
                       QStringLiteral("orchestrator"), QStringLiteral("running"),
                       QStringLiteral("llama-3.1-70b-instruct")));
    m_nodes->upsert(mk(1, QStringLiteral("n-1"), QStringLiteral("Coder · auth refactor"),
                       QStringLiteral("worker"), QStringLiteral("running"),
                       QStringLiteral("qwen2.5-coder-32b")));
    m_nodes->upsert(mk(1, QStringLiteral("n-2"), QStringLiteral("Researcher · vector DBs"),
                       QStringLiteral("worker"), QStringLiteral("idle"),
                       QStringLiteral("mixtral-8x7b")));
    m_nodes->upsert(mk(2, QStringLiteral("n-2-1"), QStringLiteral("Sub · benchmark crawl"),
                       QStringLiteral("worker"), QStringLiteral("paused"),
                       QStringLiteral("mistral-7b-instruct")));
    m_nodes->upsert(mk(1, QStringLiteral("n-3"), QStringLiteral("Coder · CI migration"),
                       QStringLiteral("worker"), QStringLiteral("running"),
                       QStringLiteral("qwen2.5-coder-32b")));
}

QObject* MockFleetTree::nodes() const { return m_nodes; }

void MockFleetTree::setStatus(const QString& id, const QString& status)
{
    const int row = m_nodes->indexOfId(id);
    if (row < 0) {
        return;
    }
    QVariantMap n = m_nodes->at(row);
    n[QStringLiteral("status")] = status;
    m_nodes->upsert(n);
    emit changed();
}

void MockFleetTree::pause(const QString& id) { setStatus(id, QStringLiteral("paused")); }
void MockFleetTree::resume(const QString& id) { setStatus(id, QStringLiteral("running")); }

} // namespace fleet
