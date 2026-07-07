// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Channels page projection (TabModel::Channels): the Events-IO account manager
// (story 04, read surface EIO-1/3/8/9) mirrored from ChannelsPage.qml - the
// configured accounts with Pidgin-style status dots (Presence), each account's
// last-known rooms (Transports.conversations), and the "Add channel" adapter
// picker. Read-only in both shells this slice: connecting (EIO-2) and
// disconnecting (EIO-7) are deferred. One focused TU for the Channels
// workstream; the dispatch stays in TuiPageHub::pageMarkdownForKind.

#include "daemonnet/idaemonnet.h"
#include "transports/ipresence_service.h"
#include "transports/itransport_registry.h"
#include "tui_page_hub.h"

// [waveB:app-v30] D1: friendly copy for the coarse disconnect-reason token (parity with the GUI
// ChannelsPage reasonLabel). The node's `message` is authoritative and rendered verbatim; this
// only labels the token when no message is present.
static QString channelReasonLabel(const QString& token) {
    if (token == QLatin1String("user_requested")) {
        return TuiPageHub::tr("Disconnected by request");
    }
    if (token == QLatin1String("network_error")) {
        return TuiPageHub::tr("Network error");
    }
    if (token == QLatin1String("authentication_failed")) {
        return TuiPageHub::tr("Authentication failed");
    }
    if (token == QLatin1String("replaced_by_other_client")) {
        return TuiPageHub::tr("Replaced by another client");
    }
    if (token == QLatin1String("invalid_settings")) {
        return TuiPageHub::tr("Invalid settings");
    }
    if (token == QLatin1String("certificate_error")) {
        return TuiPageHub::tr("Certificate error");
    }
    if (token == QLatin1String("other")) {
        return TuiPageHub::tr("Disconnected");
    }
    return {};
}

QList<QVariantMap> TuiPageHub::channelAccountRows() const {
    QList<QVariantMap> rows;
    if (m_deps.transportRegistry == nullptr) {
        return rows;
    }
    for (const QVariant& v : m_deps.transportRegistry->instances()) {
        QVariantMap a = v.toMap();
        const QString transport = a.value(QStringLiteral("transport")).toString();
        // `id` drives the disconnect/remove keys; `connection` is the live state (presence seam).
        a[QStringLiteral("id")] = transport;
        a[QStringLiteral("connection")] = m_deps.presence != nullptr
                                              ? m_deps.presence->connectionState(transport)
                                              : QStringLiteral("offline");
        rows.append(a);
    }
    return rows;
}

QString TuiPageHub::buildChannelsMarkdown(int sel) const {
    QString md;
    md += tr("# Channels\n\n");

    // The transports seams can be absent (a stripped-down service graph);
    // degrade to an explicit note instead of rendering an empty page.
    if (m_deps.transportRegistry == nullptr) {
        md += tr("_Channels are unavailable: the transports seam is not wired "
                 "in this mode._\n");
        return md;
    }

    // [waveB:app-v30] D1: connect ('c'), disconnect ('d'), remove ('x', confirmed) are all live.
    // B2: room membership is the node's; newly-joined rooms surface here on refresh and are badged.
    md += tr("Events-IO transport accounts and their live rooms, shared with "
             "the GUI. **j/k** move · **c** connect · **d** disconnect · **x** remove account. "
             "Room invites are handled by the node; newly-joined rooms appear here "
             "automatically.\n\n");

    // GUI status-dot mapping (ChannelsPage.qml dotColor): accent=connected,
    // warning=connecting/disconnecting, danger=error, muted=offline/unknown.
    const auto dotFor = [](const QString& conn) {
        if (conn == QLatin1String("connected")) {
            return QStringLiteral("●");
        }
        if (conn == QLatin1String("connecting")) {
            return QStringLiteral("◐");
        }
        // [waveB:app-v30] D1: transient teardown, rendered verbatim.
        if (conn == QLatin1String("disconnecting")) {
            return QStringLiteral("◑");
        }
        if (conn == QLatin1String("error")) {
            return QStringLiteral("✖");
        }
        return QStringLiteral("○"); // offline / unknown
    };
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    // --- Connected accounts (EIO-3 list + EIO-9 status dots) ----------------
    md += tr("## Accounts\n\n");
    const QList<QVariantMap> accounts = channelAccountRows();
    if (accounts.isEmpty()) {
        md += tr("_No channels connected._\n");
    }
    for (int i = 0; i < accounts.size(); ++i) {
        const QVariantMap& a = accounts.at(i);
        const QString transport = a.value(QStringLiteral("transport")).toString();
        const QString displayName = a.value(QStringLiteral("displayName")).toString();
        const QString family = a.value(QStringLiteral("family")).toString();
        const QString boundProfile = a.value(QStringLiteral("boundProfile")).toString();
        const QString conn = a.value(QStringLiteral("connection")).toString();
        const QString sub =
            boundProfile.isEmpty() ? family : tr("%1 · %2").arg(family, boundProfile);
        md += tr("- %1%2 **%3** — %4 · %5\n")
                  .arg(mark(i), dotFor(conn), displayName.isEmpty() ? transport : displayName, sub,
                       conn);
        // [waveB:app-v30] D1: node-reported disconnect provenance. The node's verbatim `message`
        // wins; otherwise the coarse reason token's friendly label. `fatal` adds a re-auth hint.
        const QString message = a.value(QStringLiteral("connectionMessage")).toString();
        const QString reasonText =
            message.isEmpty()
                ? channelReasonLabel(a.value(QStringLiteral("connectionReason")).toString())
                : message;
        if (!reasonText.isEmpty()) {
            md += tr("  - %1\n").arg(reasonText);
        }
        if (a.value(QStringLiteral("fatal")).toBool()) {
            md += tr("  - _Re-authentication required — reconnect with 'c'._\n");
        }

        // --- This account's rooms (EIO-8: last-known ConvList) --------------
        const QVariantList rooms = m_deps.transportRegistry->conversations(transport);
        if (rooms.isEmpty()) {
            md += tr("  - _No rooms._\n");
        }
        for (const QVariant& rv : rooms) {
            const QVariantMap r = rv.toMap();
            const QString convId = r.value(QStringLiteral("id")).toString();
            const QString title = r.value(QStringLiteral("title")).toString();
            const QString label = title.isEmpty() ? convId : title;
            const QString kind = r.value(QStringLiteral("kind")).toString();
            // Route-pin token (B6/EIO-12): GUI parity with the room-row "⇄ session" chip.
            const QString pinned =
                m_deps.daemonNet != nullptr
                    ? (kind == QLatin1String("dm")
                           ? m_deps.daemonNet->pinnedDmSessionFor(transport, convId)
                           : m_deps.daemonNet->pinnedSessionFor(transport, convId))
                    : QString();
            QString line =
                kind.isEmpty() ? tr("  - %1").arg(label) : tr("  - %1 · %2").arg(label, kind);
            if (!pinned.isEmpty()) {
                line += tr(" · ⇄ `%1`").arg(pinned);
            }
            // [wave2:app-channels-liveness] B2: badge a room the node surfaced after the baseline
            // (e.g. an auto-accepted invite) — GUI parity with the room-row "new" chip.
            if (m_deps.transportRegistry->isNewConversation(transport, convId)) {
                line += tr(" · ✦ new");
            }
            md += line + QLatin1Char('\n');
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
        // [waveB:app-v30] D3: read-only node-labeled policy rows (verbatim; never keyed on `key`).
        for (const QVariant& pv : p.value(QStringLiteral("policies")).toList()) {
            const QVariantMap policy = pv.toMap();
            md += tr("  - %1: %2\n")
                      .arg(policy.value(QStringLiteral("label")).toString(),
                           policy.value(QStringLiteral("value")).toString());
        }
    }

    return md;
}
