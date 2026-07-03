// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Per-agent Memory page projection (TabModel::Memory): the shared Mnemosyne
// view-models rendered as markdown + an adjacency listing. Split out of
// tui_page_hub.cpp; the dispatch stays in TuiPageHub::pageMarkdownForKind.

#include "memory/imemory_service.h"
#include "memory_graph_model.h"
#include "memory_list_model.h"
#include "memory_stats_model.h"
#include "memory_timeline_model.h"
#include "tui_page_hub.h"

QString TuiPageHub::buildMemoryMarkdown() const {
    QString md;
    const QString agent = m_deps.memory != nullptr ? m_deps.memory->profile() : QString();
    md += tr("# Memory - %1\n\n").arg(agent.isEmpty() ? tr("agent") : agent);
    md += tr("Mnemosyne memory for the active scope, shared with the GUI. The "
             "knowledge graph renders as a node-link diagram in the GUI; here it is "
             "an adjacency listing.\n\n");

    if (m_deps.memStats != nullptr) {
        md += tr("## Overview\n\n");
        md += tr("- Working: **%1** · Episodic: **%2** · Scratchpad: **%3**\n")
                  .arg(m_deps.memStats->working())
                  .arg(m_deps.memStats->episodic())
                  .arg(m_deps.memStats->scratchpad());
        md += tr("- Facts: **%1** · Conflicts: **%2**\n\n")
                  .arg(m_deps.memStats->facts())
                  .arg(m_deps.memStats->conflicts());

        const auto gauges = [&md](const QString& title, const QVariantList& rows) {
            if (rows.isEmpty())
                return;
            md += QStringLiteral("**%1**\n\n").arg(title);
            for (const QVariant& v : rows) {
                const QVariantMap r = v.toMap();
                const double frac = r.value(QStringLiteral("fraction")).toDouble();
                const int filled = qBound(0, qRound(frac * 12.0), 12);
                const QString bar = QString(filled, QChar('#')) + QString(12 - filled, QChar('-'));
                md += QStringLiteral("- `%1` %2 %3\n")
                          .arg(bar, r.value(QStringLiteral("key")).toString())
                          .arg(r.value(QStringLiteral("count")).toInt());
            }
            md += QStringLiteral("\n");
        };
        gauges(tr("By source"), m_deps.memStats->bySource());
        gauges(tr("By veracity"), m_deps.memStats->byVeracity());
        gauges(tr("By lifecycle"), m_deps.memStats->byDegradation());
    }

    if (m_deps.memList != nullptr) {
        md += tr("## Memories\n\n");
        const int n = m_deps.memList->rowCount();
        if (n == 0)
            md += tr("_No memories in scope._\n\n");
        for (int i = 0; i < n; ++i) {
            const QVariantMap e = m_deps.memList->entryAt(i);
            md += tr("- **%1** _(%2 · %3 · imp %4)_\n")
                      .arg(e.value(QStringLiteral("content")).toString(),
                           e.value(QStringLiteral("tier")).toString(),
                           e.value(QStringLiteral("veracity")).toString())
                      .arg(e.value(QStringLiteral("importance")).toDouble(), 0, 'f', 2);
        }
        md += QStringLiteral("\n");
    }

    if (m_deps.memGraph != nullptr) {
        md += tr("## Graph adjacency\n\n");
        const QVariantList edges = m_deps.memGraph->edges();
        if (edges.isEmpty())
            md += tr("_No edges in scope._\n\n");
        for (const QVariant& v : edges) {
            const QVariantMap e = v.toMap();
            md += QStringLiteral("- `%1` --%2--> `%3`\n")
                      .arg(e.value(QStringLiteral("source")).toString(),
                           e.value(QStringLiteral("edgeType")).toString(),
                           e.value(QStringLiteral("target")).toString());
        }
        md += QStringLiteral("\n");
    }

    if (m_deps.memTimeline != nullptr) {
        md += tr("## Timeline\n\n");
        const int n = m_deps.memTimeline->rowCount();
        for (int i = 0; i < n; ++i) {
            const QModelIndex idx = m_deps.memTimeline->index(i);
            const bool header =
                m_deps.memTimeline->data(idx, memoryui::MemoryTimelineModel::IsHeaderRole).toBool();
            if (header) {
                md += QStringLiteral("\n**%1**\n\n")
                          .arg(m_deps.memTimeline
                                   ->data(idx, memoryui::MemoryTimelineModel::GroupKeyRole)
                                   .toString());
            } else {
                md +=
                    QStringLiteral("- _%1_ %2\n")
                        .arg(m_deps.memTimeline->data(idx, memoryui::MemoryTimelineModel::KindRole)
                                 .toString(),
                             m_deps.memTimeline
                                 ->data(idx, memoryui::MemoryTimelineModel::SummaryRole)
                                 .toString());
            }
        }
        md += QStringLiteral("\n");
    }

    return md;
}
