// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "tools/mock_tool_inventory.h"

#include "uimodels/variant_list_model.h"

namespace tools {

namespace {
QVariantMap toolRow(const QString& name, const QString& description, bool enabled,
                    const QString& requires_, const QString& requirementLabel,
                    bool requirementIsCredential) {
    QVariantMap row;
    row[QStringLiteral("name")] = name;
    row[QStringLiteral("description")] = description;
    row[QStringLiteral("enabled")] = enabled;
    row[QStringLiteral("requires")] = requires_;
    row[QStringLiteral("requirementLabel")] = requirementLabel;
    row[QStringLiteral("requirementIsCredential")] = requirementIsCredential;
    return row;
}
} // namespace

MockToolInventory::MockToolInventory(QObject* parent)
    : IToolInventory(parent), m_tools(new uimodels::VariantListModel(this)) {
    connect(m_tools, &uimodels::VariantListModel::countChanged, this, &IToolInventory::changed);
    m_tools->setRows(QList<QVariantMap>{
        toolRow(QStringLiteral("fs"), tr("Read and edit workspace files"), true, QString(),
                QString(), false),
        toolRow(QStringLiteral("shell"), tr("Run shell commands (approval-gated)"), true, QString(),
                QString(), false),
        toolRow(QStringLiteral("web_search"), tr("Search the web"), false, QStringLiteral("tavily"),
                tr("Needs a Tavily API key"), true),
        toolRow(QStringLiteral("browser"), tr("Drive a real browser"), false,
                QStringLiteral("browser"), tr("Needs the browser build feature"), false),
    });
}

QObject* MockToolInventory::tools() const {
    return m_tools;
}

int MockToolInventory::count() const {
    return m_tools->count();
}

void MockToolInventory::refresh() {
    // Canned data — nothing to re-query.
}

} // namespace tools
