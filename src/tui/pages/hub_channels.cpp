// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Channels page projection (TabModel::Channels): the Events-IO account manager
// (story 04, read surface EIO-1/3/8/9) mirrored from ChannelsPage.qml - the
// configured accounts with Pidgin-style status dots (Presence), each account's
// last-known rooms (Transports.conversations), and the "Add channel" adapter
// picker. Read-only in both shells this slice: connecting (EIO-2) and
// disconnecting (EIO-7) are deferred. One focused TU for the Channels
// workstream; the dispatch stays in TuiPageHub::pageMarkdownForKind.

#include "transports/ipresence_service.h"
#include "transports/itransport_registry.h"
#include "tui_page_hub.h"

QString TuiPageHub::buildChannelsMarkdown() const {
    QString md;
    md += tr("# Channels\n\n");

    // The transports seams can be absent (a stripped-down service graph);
    // degrade to an explicit note instead of rendering an empty page.
    if (m_deps.transportRegistry == nullptr) {
        md += tr("_Channels are unavailable: the transports seam is not wired "
                 "in this mode._\n");
        return md;
    }

    md += tr("Events-IO transport accounts and their live rooms, shared with "
             "the GUI. Read-only in both shells this slice - connecting is "
             "deferred (EIO-2).\n\n");

    // GUI status-dot mapping (ChannelsPage.qml dotColor): accent=connected,
    // warning=connecting, danger=error, muted=offline/unknown.
    const auto dotFor = [](const QString& conn) {
        if (conn == QLatin1String("connected")) {
            return QStringLiteral("●");
        }
        if (conn == QLatin1String("connecting")) {
            return QStringLiteral("◐");
        }
        if (conn == QLatin1String("error")) {
            return QStringLiteral("✖");
        }
        return QStringLiteral("○"); // offline / unknown
    };

    // --- Connected accounts (EIO-3 list + EIO-9 status dots) ----------------
    md += tr("## Accounts\n\n");
    const QVariantList accounts = m_deps.transportRegistry->instances();
    if (accounts.isEmpty()) {
        md += tr("_No channels connected._\n");
    }
    for (const QVariant& v : accounts) {
        const QVariantMap a = v.toMap();
        const QString transport = a.value(QStringLiteral("transport")).toString();
        const QString displayName = a.value(QStringLiteral("displayName")).toString();
        const QString family = a.value(QStringLiteral("family")).toString();
        const QString boundProfile = a.value(QStringLiteral("boundProfile")).toString();
        // Live per-account connection state ("offline" when the presence seam
        // is absent, matching the GUI's muted-dot fallback).
        const QString conn = m_deps.presence != nullptr
                                 ? m_deps.presence->connectionState(transport)
                                 : QStringLiteral("offline");
        const QString sub =
            boundProfile.isEmpty() ? family : tr("%1 · %2").arg(family, boundProfile);
        md += tr("- %1 **%2** — %3 · %4\n")
                  .arg(dotFor(conn), displayName.isEmpty() ? transport : displayName, sub, conn);

        // --- This account's rooms (EIO-8: last-known ConvList) --------------
        const QVariantList rooms = m_deps.transportRegistry->conversations(transport);
        if (rooms.isEmpty()) {
            md += tr("  - _No rooms._\n");
        }
        for (const QVariant& rv : rooms) {
            const QVariantMap r = rv.toMap();
            const QString title = r.value(QStringLiteral("title")).toString();
            const QString label =
                title.isEmpty() ? r.value(QStringLiteral("id")).toString() : title;
            const QString kind = r.value(QStringLiteral("kind")).toString();
            md += kind.isEmpty() ? tr("  - %1\n").arg(label) : tr("  - %1 · %2\n").arg(label, kind);
        }
    }
    md += QLatin1Char('\n');

    // --- Add channel (EIO-1 adapter picker; connect deferred) ---------------
    md += tr("## Add channel\n\n");
    const QVariantList adapters = m_deps.transportRegistry->availableAdapters();
    if (adapters.isEmpty()) {
        md += tr("_Connect to a daemon to see available channel types._\n");
    }
    for (const QVariant& v : adapters) {
        const QVariantMap p = v.toMap();
        md += tr("- **%1** (`%2`)\n")
                  .arg(p.value(QStringLiteral("displayName")).toString(),
                       p.value(QStringLiteral("family")).toString());
    }

    return md;
}
