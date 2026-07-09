// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Profiles list (TabModel::Profiles) + per-agent Profile editor page
// (TabModel::Profile) projections. Split out of tui_page_hub.cpp so the
// profile-editor workstream owns one focused TU; the dispatch stays in
// TuiPageHub::pageMarkdownForKind.

#include "profiles/iprofile_store.h"
#include "tui_page_hub.h"
#include "uimodels/variant_list_model.h"

QString TuiPageHub::engineToken(const QVariantMap& row) {
    const QString engine = row.value(QStringLiteral("engine")).toString();
    if (engine.isEmpty()) {
        return tr("Native"); // rows without the field are core-engine by construction
    }
    if (engine == QStringLiteral("Foreign")) {
        // Mirror the GUI ProfilesPage chip: the foreign agent name (the protocol suffix is only
        // shown where the catalog is joined - see EngineIdentity / the engine picker).
        const QString agent = row.value(QStringLiteral("acpAgent")).toString();
        return agent.isEmpty() ? tr("Foreign") : agent;
    }
    return tr("Native");
}

QString TuiPageHub::buildProfilesMarkdown(int sel) const {
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_deps.profiles->profiles());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Profiles\n\n");
    md += tr("Agent profiles, shared with the GUI. **j/k** move · **Enter** "
             "set default · **e** edit (provider / model / prompt / skills / "
             "tools) · **n** clone · **a** new agent · **x** delete.\n\n");

    if (model != nullptr) {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& p = rows.at(i);
            md += tr("## %1%2%3\n\n")
                      .arg(mark(i), p.value(QStringLiteral("name")).toString(),
                           p.value(QStringLiteral("isDefault")).toBool() ? tr(" (default)")
                                                                         : QString());
            // Engine identity (C3/ENG-7): the same chip the GUI rows render — Native core vs the
            // foreign ACP agent name.
            md += tr("- Engine: %1\n").arg(engineToken(p));
            // Provenance (phase H): mirror the GUI "agent-authored" badge + owner. Consumed
            // verbatim from the node-stamped row keys (createdByIsAgent / owner); never re-derived.
            if (p.value(QStringLiteral("createdByIsAgent")).toBool()) {
                const QString owner = p.value(QStringLiteral("owner")).toString();
                md += owner.isEmpty()
                          ? tr("- Provenance: agent-authored\n")
                          : tr("- Provenance: agent-authored (owner `%1`)\n").arg(owner);
            }
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
    md += tr("Agent == profile. Memory lives in this agent's bank (`%1`). "
             "Press **e** to edit this profile.\n\n")
              .arg(profileRef);

    md += tr("## Engine\n\n");
    md += tr("- Engine: %1\n").arg(engineToken(p));
    md += tr("- Provider: **%1**\n").arg(val(QStringLiteral("provider"), QStringLiteral("-")));
    md += tr("- Model: **%1**\n").arg(val(QStringLiteral("model"), QStringLiteral("-")));
    md += tr("- Base URL: %1\n").arg(val(QStringLiteral("baseUrl"), tr("(provider default)")));
    md += tr("- Context engine: %1\n\n")
              .arg(val(QStringLiteral("contextEngine"), QStringLiteral("lcm")));

    md += tr("## Memory\n\n");
    md += tr("- Memory provider: **%1**\n\n")
              .arg(val(QStringLiteral("memoryProvider"), QStringLiteral("mnemosyne")));

    // Persona (SOUL.md), via the profile store's persona seam — Core-engine only:
    // a foreign agent owns its own prompt, so the section is omitted for foreign
    // profiles (GUI AgentProfilePage parity).
    if (p.value(QStringLiteral("engine")).toString() != QStringLiteral("Foreign")) {
        md += tr("## Persona\n\n");
        const QString soul = m_deps.profiles->soul(profileRef);
        md += (soul.isEmpty() ? QStringLiteral("-") : soul) + QStringLiteral("\n\n");
    }

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
