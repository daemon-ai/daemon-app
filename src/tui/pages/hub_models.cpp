// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Models hub projection (TabModel::Models). Split out of tui_page_hub.cpp so
// the models workstream owns one focused TU; the dispatch stays in
// TuiPageHub::pageMarkdownForKind.

#include "hub_detail.h"
#include "models/imodel_catalog.h"
#include "tui_page_hub.h"

QString TuiPageHub::buildModelsMarkdown(int sel) const {
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Models\n\n");
    md += tr("Installed models, shared with the GUI. **j/k** move · **Enter** "
             "activates · **x** removes · **d** download a model (repo \u2192 quant).\n\n");

    md += tr("## Installed\n\n");
    const auto installed = hubdetail::rowsOfModel(m_deps.modelCatalog->installed());
    if (installed.isEmpty()) {
        md += tr("_None installed._\n\n");
    } else {
        for (int i = 0; i < installed.size(); ++i) {
            const QVariantMap& m = installed.at(i);
            // Local models carry a quant + on-disk size label; cloud models show their provider.
            const QString detail =
                m.value(QStringLiteral("quant")).toString().isEmpty()
                    ? m.value(QStringLiteral("provider")).toString()
                    : (m.value(QStringLiteral("quant")).toString() + QStringLiteral(", ") +
                       m.value(QStringLiteral("sizeLabel")).toString());
            md += tr("- %1**%2** (%3)%4\n")
                      .arg(mark(i), m.value(QStringLiteral("name")).toString(), detail,
                           m.value(QStringLiteral("active")).toBool() ? tr(" — **active**")
                                                                      : QString());
        }
        md += QLatin1Char('\n');
    }

    const auto downloads = hubdetail::rowsOfModel(m_deps.modelCatalog->downloads());
    if (!downloads.isEmpty()) {
        md += tr("## Downloads\n\n");
        for (const QVariantMap& m : downloads) {
            const int pct =
                static_cast<int>(m.value(QStringLiteral("progress")).toDouble() * 100.0);
            md += tr("- %1 — %2%  · %3\n")
                      .arg(m.value(QStringLiteral("name")).toString())
                      .arg(pct)
                      .arg(m.value(QStringLiteral("state")).toString());
        }
        md += QLatin1Char('\n');
    }

    md += tr("## Discover\n\n");
    const auto discover = hubdetail::rowsOfModel(m_deps.modelCatalog->discover());
    if (discover.isEmpty()) {
        md += tr("_Press **d** to search repos and download a model._\n");
    }
    for (const QVariantMap& m : discover) {
        md += tr("- %1 — %2 \u00b7 \u2193 %3\n")
                  .arg(m.value(QStringLiteral("name")).toString(),
                       m.value(QStringLiteral("params")).toString(),
                       QString::number(m.value(QStringLiteral("downloads")).toLongLong()));
    }

    md += tr("\n## Providers\n\n");
    const QVariantList provs = m_deps.modelCatalog->providers();
    for (const QVariant& v : provs) {
        const QVariantMap m = v.toMap();
        md += tr("- %1 (%2)%3\n")
                  .arg(m.value(QStringLiteral("name")).toString(),
                       m.value(QStringLiteral("kind")).toString(),
                       m.value(QStringLiteral("configured")).toBool() ? tr(" — configured")
                                                                      : QString());
    }

    return md;
}
