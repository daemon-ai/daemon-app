// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Accounts page projection (TabModel::Accounts). Split out of tui_page_hub.cpp
// so the accounts-wizard workstream owns one focused TU; the dispatch stays in
// TuiPageHub::pageMarkdownForKind.

#include "accounts/iaccounts_service.h"
#include "tui_page_hub.h"
#include "uimodels/variant_list_model.h"

QString TuiPageHub::buildAccountsMarkdown(int sel) const {
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_deps.accounts->accounts());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Accounts\n\n");
    md += tr("Connected provider accounts, shared with the GUI. **j/k** move · "
             "**a** add account · **R**/**Enter** re-auth · **x** remove.\n\n");

    md += tr("## Connected\n\n");
    if (model == nullptr || model->count() == 0) {
        md += tr("_No accounts._\n\n");
    } else {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& a = rows.at(i);
            md += tr("- %1**%2** — %3 (%4) · %5\n")
                      .arg(mark(i), a.value(QStringLiteral("label")).toString(),
                           a.value(QStringLiteral("kind")).toString() == QLatin1String("oauth")
                               ? tr("OAuth")
                               : tr("API key"),
                           a.value(QStringLiteral("status")).toString(),
                           a.value(QStringLiteral("detail")).toString());
        }
        md += QLatin1Char('\n');
    }

    md += tr("## Available providers\n\n");
    for (const QVariant& v : m_deps.accounts->availableProviders()) {
        const QVariantMap p = v.toMap();
        md += tr("- %1 (%2)\n")
                  .arg(p.value(QStringLiteral("name")).toString(),
                       p.value(QStringLiteral("kinds")).toStringList().join(QStringLiteral(", ")));
    }

    return md;
}
