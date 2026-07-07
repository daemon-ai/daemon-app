// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_tool_inventory.h"

#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"

namespace daemonapp::daemon {

namespace {
// Credential-shaped requirement tokens: the unmet prerequisite is a secret the wave-1 auth stack
// owns, so the UI deep-links to Connection rather than offering inline key entry (coordinator Q3).
bool requirementIsCredential(const QString& token) {
    return token == QStringLiteral("tavily") || token == QStringLiteral("firecrawl");
}

// Friendly, honest copy for a known requirement token; unknown tokens are shown verbatim so the
// app never fabricates a reason the node did not send.
QString requirementLabel(const QString& token) {
    if (token.isEmpty()) {
        return {};
    }
    if (token == QStringLiteral("tavily")) {
        return tools::IToolInventory::tr("Needs a Tavily API key");
    }
    if (token == QStringLiteral("firecrawl")) {
        return tools::IToolInventory::tr("Needs a Firecrawl API key");
    }
    if (token == QStringLiteral("vision")) {
        return tools::IToolInventory::tr("Needs the vision build feature");
    }
    if (token == QStringLiteral("browser")) {
        return tools::IToolInventory::tr("Needs the browser build feature");
    }
    return tools::IToolInventory::tr("Needs %1").arg(token);
}
} // namespace

DaemonToolInventory::DaemonToolInventory(ToolRepository* repository, QObject* parent)
    : tools::IToolInventory(parent), m_repository(repository),
      m_tools(new uimodels::VariantListModel(this)) {
    connect(m_tools, &uimodels::VariantListModel::countChanged, this,
            &tools::IToolInventory::changed);
    if (m_repository != nullptr) {
        connect(m_repository, &ToolRepository::toolsRefreshed, this, &DaemonToolInventory::rebuild);
    }
}

QObject* DaemonToolInventory::tools() const {
    return m_tools;
}

int DaemonToolInventory::count() const {
    return m_tools->count();
}

void DaemonToolInventory::refresh() {
    if (m_repository != nullptr) {
        m_repository->refreshTools();
    }
}

// [waveB:app-v30] D4: forward the toggle to the repository; the node-authoritative re-fetch
// (toolsRefreshed -> rebuild) renders the actual overlay result.
void DaemonToolInventory::setEnabled(const QString& name, bool enabled) {
    if (m_repository != nullptr) {
        m_repository->setEnabled(name, enabled);
    }
}

void DaemonToolInventory::rebuild() {
    QList<QVariantMap> rows;
    rows.reserve(m_repository->tools().size());
    for (const DecodedToolInfo& info : m_repository->tools()) {
        QVariantMap row;
        row[QStringLiteral("name")] = info.name;
        row[QStringLiteral("description")] = info.description;
        row[QStringLiteral("enabled")] = info.enabled;
        row[QStringLiteral("requires")] = info.requires_;
        row[QStringLiteral("requirementLabel")] = requirementLabel(info.requires_);
        row[QStringLiteral("requirementIsCredential")] = requirementIsCredential(info.requires_);
        rows.append(row);
    }
    m_tools->setRows(rows);
}

} // namespace daemonapp::daemon
