// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Profiles list (TabModel::Profiles) + per-agent Profile editor page
// (TabModel::Profile) projections. Split out of tui_page_hub.cpp so the
// profile-editor workstream owns one focused TU; the dispatch stays in
// TuiPageHub::pageMarkdownForKind.

#include "profiles/iprofile_store.h"
#include "tui_page_hub.h"
#include "uimodels/variant_list_model.h"

QString TuiPageHub::buildProfilesMarkdown(int sel) const {
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_deps.profiles->profiles());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Profiles\n\n");
    md += tr("Agent profiles, shared with the GUI. **j/k** move · **Enter** "
             "set default · **x** delete. Use the GUI editor for model / "
             "prompt / skills.\n\n");

    if (model != nullptr) {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& p = rows.at(i);
            md += tr("## %1%2%3\n\n")
                      .arg(mark(i), p.value(QStringLiteral("name")).toString(),
                           p.value(QStringLiteral("isDefault")).toBool() ? tr(" (default)")
                                                                         : QString());
            md += tr("- Model: `%1`\n").arg(p.value(QStringLiteral("model")).toString());
            const QString desc = p.value(QStringLiteral("description")).toString();
            if (!desc.isEmpty()) {
                md += QStringLiteral("- %1\n").arg(desc);
            }
            md += tr("- Skills: %1\n")
                      .arg(p.value(QStringLiteral("skills"))
                               .toStringList()
                               .join(QStringLiteral(", ")));
            md +=
                tr("- Tools: %1\n\n")
                    .arg(
                        p.value(QStringLiteral("tools")).toStringList().join(QStringLiteral(", ")));
        }
    }

    return md;
}

QString TuiPageHub::buildProfileMarkdown(const QString& profileRef) const {
    QString md;
    if (profileRef.isEmpty() || m_deps.profiles == nullptr) {
        md += tr("# Profile\n\n_No agent selected._\n");
        return md;
    }
    const QVariantMap p = m_deps.profiles->profile(profileRef);
    const auto val = [&p](const QString& key, const QString& fallback) {
        const QString v = p.value(key).toString();
        return v.isEmpty() ? fallback : v;
    };

    md += tr("# Profile - %1\n\n").arg(val(QStringLiteral("name"), profileRef));
    md += tr("Agent == profile. Memory lives in this agent's bank (`%1`).\n\n").arg(profileRef);

    md += tr("## Engine\n\n");
    md += tr("- Provider: **%1**\n").arg(val(QStringLiteral("provider"), QStringLiteral("-")));
    md += tr("- Model: **%1**\n").arg(val(QStringLiteral("model"), QStringLiteral("-")));
    md += tr("- Base URL: %1\n").arg(val(QStringLiteral("baseUrl"), tr("(provider default)")));
    md += tr("- Context engine: %1\n\n")
              .arg(val(QStringLiteral("contextEngine"), QStringLiteral("lcm")));

    md += tr("## Memory\n\n");
    md += tr("- Memory provider: **%1**\n\n")
              .arg(val(QStringLiteral("memoryProvider"), QStringLiteral("mnemosyne")));

    md += tr("## Persona\n\n");
    md += val(QStringLiteral("systemPrompt"), QStringLiteral("-")) + QStringLiteral("\n\n");

    const auto chips = [&md, &p](const QString& title, const QString& key) {
        const QStringList items = p.value(key).toStringList();
        md += QStringLiteral("## %1\n\n").arg(title);
        if (items.isEmpty()) {
            md += tr("_none_\n\n");
            return;
        }
        for (const QString& it : items)
            md += QStringLiteral("- `%1`\n").arg(it);
        md += QStringLiteral("\n");
    };
    chips(tr("Tool allowlist"), QStringLiteral("toolAllowlist"));
    chips(tr("Skills"), QStringLiteral("skills"));
    return md;
}
