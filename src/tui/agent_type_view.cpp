// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "agent_type_view.h"

#include "daemon/repositories.h"

#include <QAbstractItemModel>

namespace {
// Match the GUI badge wording (translated identically) so the two surfaces read the same.
QString rowLabel(const daemonapp::daemon::DecodedAgentEntry& entry) {
    QString label = entry.name;
    // Protocol suffix (v29): stream-json has no ACP version; ACP shows its probed version.
    if (entry.protocol == QStringLiteral("StreamJson")) {
        label += QStringLiteral("  (stream-json)");
    } else if (!entry.version.isEmpty()) {
        label += QStringLiteral("  (ACP %1)").arg(entry.version);
    }
    label += entry.installed ? QObject::tr("  [installed]") : QObject::tr("  [not installed]");
    return label;
}
} // namespace

AgentTypeView::AgentTypeView(daemonapp::daemon::AgentRepository* repo, Tui::ZWidget* parent)
    : Tui::ZListView(parent), m_repo(repo) {
    if (m_repo != nullptr) {
        connect(m_repo, &daemonapp::daemon::AgentRepository::catalogRefreshed, this,
                &AgentTypeView::rebuild);
    }
    // ZListView has no row-change signal of its own here; consumers re-read on Enter/commit and
    // on our rebuild-driven selectionChanged. Rebuild seeds row 0 (native).
    rebuild();
}

void AgentTypeView::refresh() {
    if (m_repo != nullptr) {
        m_repo->refreshCatalog();
        m_repo->discover(); // fresh installed badges beside the durable catalog
    }
    rebuild();
}

QString AgentTypeView::selectedAgent() const {
    const int row = currentIndex().isValid() ? currentIndex().row() : 0;
    if (row <= 0 || row > m_agents.size()) {
        return {};
    }
    // An uninstalled foreign row is not a valid selection: report native (the GUI equivalent
    // keeps such rows unselectable; the ZListView cursor can rest on them, so gate here).
    if (!m_agentInstalled.at(row - 1)) {
        return {};
    }
    return m_agents.at(row - 1);
}

bool AgentTypeView::selectedInstalled() const {
    const int row = currentIndex().isValid() ? currentIndex().row() : 0;
    if (row <= 0 || row > m_agents.size()) {
        return true; // the native engine is always available
    }
    return m_agentInstalled.at(row - 1);
}

void AgentTypeView::rebuild() {
    const int keep = currentIndex().isValid() ? currentIndex().row() : 0;
    m_agents.clear();
    m_agentInstalled.clear();
    QStringList items{tr("daemon-core (native)")};
    if (m_repo != nullptr) {
        for (const daemonapp::daemon::DecodedAgentEntry& e : m_repo->entries()) {
            m_agents << e.name;
            m_agentInstalled << e.installed;
            items << rowLabel(e);
        }
    }
    setItems(items);
    if (model() != nullptr) {
        const int row = qBound(0, keep, static_cast<int>(items.size()) - 1);
        setCurrentIndex(model()->index(row, 0));
    }
    emit selectionChanged();
}
