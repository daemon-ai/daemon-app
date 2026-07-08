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
#include "transports/icontacts_service.h"
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

QList<QVariantMap> TuiPageHub::channelRows() const {
    // [acct-mgmt] A typed flattened row list: each account row is immediately followed by its
    // room rows (rowKind "account" | "room"), so j/k walks accounts AND their rooms and the
    // row-contextual keys act on the selected row's kind (GUI parity: the account card + its
    // expandable room rows). `id` is the transport for an account and the conversation id for a
    // room; both carry `transport` so a room key knows its account.
    QList<QVariantMap> rows;
    if (m_deps.transportRegistry == nullptr) {
        return rows;
    }
    // [acct-mgmt] Resolve a family's rosterOps.list (wire v34) — the Contacts group shows ONLY when
    // the node reported it (no legacy coarse fallback for contacts, GUI canRosterOp parity).
    const auto familyHasRosterList = [this](const QString& family) {
        for (const QVariant& v : m_deps.transportRegistry->availableAdapters()) {
            const QVariantMap a = v.toMap();
            if (a.value(QStringLiteral("family")).toString() == family) {
                return a.contains(QStringLiteral("rosterOps")) &&
                       a.value(QStringLiteral("rosterOps"))
                           .toMap()
                           .value(QStringLiteral("list"))
                           .toBool();
            }
        }
        return false;
    };
    for (const QVariant& v : m_deps.transportRegistry->instances()) {
        QVariantMap a = v.toMap();
        const QString transport = a.value(QStringLiteral("transport")).toString();
        const QString family = a.value(QStringLiteral("family")).toString();
        a[QStringLiteral("rowKind")] = QStringLiteral("account");
        a[QStringLiteral("id")] = transport;
        a[QStringLiteral("connection")] = m_deps.presence != nullptr
                                              ? m_deps.presence->connectionState(transport)
                                              : QStringLiteral("offline");
        rows.append(a);
        for (const QVariant& rv : m_deps.transportRegistry->conversations(transport)) {
            const QVariantMap r = rv.toMap();
            const QString convId = r.value(QStringLiteral("id")).toString();
            const QString title = r.value(QStringLiteral("title")).toString();
            QVariantMap row;
            row[QStringLiteral("rowKind")] = QStringLiteral("room");
            row[QStringLiteral("id")] = convId;
            row[QStringLiteral("transport")] = transport;
            row[QStringLiteral("convId")] = convId;
            row[QStringLiteral("family")] = family;
            row[QStringLiteral("roomLabel")] = title.isEmpty() ? convId : title;
            row[QStringLiteral("kind")] = r.value(QStringLiteral("kind")).toString();
            rows.append(row);
        }
        // [acct-mgmt] The Contacts group + its contact sub-rows (after the rooms), gated on
        // rosterOps.list. `a`/`f` act on the group row; the contact rows carry Enter/`a`/`x`/`m`.
        if (m_deps.contacts != nullptr && familyHasRosterList(family)) {
            const QVariantList contacts = m_deps.contacts->contacts(transport);
            QVariantMap group;
            group[QStringLiteral("rowKind")] = QStringLiteral("contactsGroup");
            group[QStringLiteral("id")] = transport;
            group[QStringLiteral("transport")] = transport;
            group[QStringLiteral("family")] = family;
            group[QStringLiteral("count")] = contacts.size();
            rows.append(group);
            for (const QVariant& cv : contacts) {
                const QVariantMap c = cv.toMap();
                const QString id = c.value(QStringLiteral("id")).toString();
                const QString name = c.value(QStringLiteral("displayName")).toString();
                QVariantMap row;
                row[QStringLiteral("rowKind")] = QStringLiteral("contact");
                row[QStringLiteral("id")] = id;
                row[QStringLiteral("transport")] = transport;
                row[QStringLiteral("family")] = family;
                row[QStringLiteral("contactId")] = id;
                row[QStringLiteral("displayName")] = name;
                row[QStringLiteral("presence")] = c.value(QStringLiteral("presence")).toString();
                rows.append(row);
            }
        }
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

    // [acct-mgmt] j/k walks accounts AND their rooms; keys are row-contextual (account vs room).
    md += tr("Events-IO transport accounts, their rooms, members and contacts, shared with the "
             "GUI. **j/k** move.\n\n"
             "Account row: **c** connect · **d** disconnect · **x** remove account · "
             "**g** join room · **n** new room · **a** add contact · **f** find people.\n"
             "Room row: **Enter** members · **i** invite · **l** leave · **x** delete · "
             "**p** pin to agent.\n"
             "Contacts group: **a** add contact · **f** find people.\n"
             "Contact row: **Enter** profile · **a** alias · **x** remove · **m** open DM.\n\n"
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

    // --- Connected accounts + their rooms (EIO-3/8/9; [acct-mgmt] selectable rooms) ----------
    md += tr("## Accounts\n\n");
    const QList<QVariantMap> rows = channelRows();
    // Count account rows for the empty-state note (a flattened list can hold rooms only after an
    // account, so an empty list means no accounts).
    if (rows.isEmpty()) {
        md += tr("_No channels connected._\n");
    }
    QString lastAccount; // to render a "no rooms" note for an account with no following room rows
    bool lastAccountHadRoom = false;
    const auto flushNoRooms = [&] {
        if (!lastAccount.isEmpty() && !lastAccountHadRoom) {
            md += tr("  - _No rooms._\n");
        }
    };
    for (int i = 0; i < rows.size(); ++i) {
        const QVariantMap& row = rows.at(i);
        const QString rowKind = row.value(QStringLiteral("rowKind")).toString();
        if (rowKind == QLatin1String("account")) {
            flushNoRooms();
            const QString transport = row.value(QStringLiteral("transport")).toString();
            const QString displayName = row.value(QStringLiteral("displayName")).toString();
            const QString family = row.value(QStringLiteral("family")).toString();
            const QString boundProfile = row.value(QStringLiteral("boundProfile")).toString();
            const QString conn = row.value(QStringLiteral("connection")).toString();
            const QString sub =
                boundProfile.isEmpty() ? family : tr("%1 · %2").arg(family, boundProfile);
            md += tr("- %1%2 **%3** — %4 · %5\n")
                      .arg(mark(i), dotFor(conn), displayName.isEmpty() ? transport : displayName,
                           sub, conn);
            // [waveB:app-v30] D1: node-reported disconnect provenance (verbatim message wins).
            const QString message = row.value(QStringLiteral("connectionMessage")).toString();
            const QString reasonText =
                message.isEmpty()
                    ? channelReasonLabel(row.value(QStringLiteral("connectionReason")).toString())
                    : message;
            if (!reasonText.isEmpty()) {
                md += tr("  - %1\n").arg(reasonText);
            }
            if (row.value(QStringLiteral("fatal")).toBool()) {
                md += tr("  - _Re-authentication required — reconnect with 'c'._\n");
            }
            lastAccount = transport;
            lastAccountHadRoom = false;
            continue;
        }
        // [acct-mgmt] Contacts group header (after the account's rooms): "~ Contacts (n)".
        if (rowKind == QLatin1String("contactsGroup")) {
            flushNoRooms();
            lastAccount.clear(); // rooms for this account are done; suppress the no-rooms note
            md += tr("  - %1~ Contacts (%2)\n")
                      .arg(mark(i))
                      .arg(row.value(QStringLiteral("count")).toInt());
            continue;
        }
        // [acct-mgmt] Contact sub-row: presence glyph + name (alias overlay) + id.
        if (rowKind == QLatin1String("contact")) {
            const QString id = row.value(QStringLiteral("contactId")).toString();
            const QString name = row.value(QStringLiteral("displayName")).toString();
            const QString presence = row.value(QStringLiteral("presence")).toString();
            const QString glyph =
                presence == QLatin1String("available") || presence == QLatin1String("streaming")
                    ? QStringLiteral("●")
                    : (presence == QLatin1String("idle") || presence == QLatin1String("away")
                           ? QStringLiteral("◐")
                           : QStringLiteral("○"));
            md += name.isEmpty() ? tr("    - %1%2 `%3`\n").arg(mark(i), glyph, id)
                                 : tr("    - %1%2 %3 · `%4`\n").arg(mark(i), glyph, name, id);
            continue;
        }
        // Room row (EIO-8): rendered as a selectable, indented sub-row.
        lastAccountHadRoom = true;
        const QString transport = row.value(QStringLiteral("transport")).toString();
        const QString convId = row.value(QStringLiteral("convId")).toString();
        const QString label = row.value(QStringLiteral("roomLabel")).toString();
        const QString kind = row.value(QStringLiteral("kind")).toString();
        const QString pinned = m_deps.daemonNet != nullptr
                                   ? (kind == QLatin1String("dm")
                                          ? m_deps.daemonNet->pinnedDmSessionFor(transport, convId)
                                          : m_deps.daemonNet->pinnedSessionFor(transport, convId))
                                   : QString();
        QString line = kind.isEmpty() ? tr("  - %1# %2").arg(mark(i), label)
                                      : tr("  - %1# %2 · %3").arg(mark(i), label, kind);
        if (!pinned.isEmpty()) {
            line += tr(" · ⇄ `%1`").arg(pinned);
        }
        if (m_deps.transportRegistry->isNewConversation(transport, convId)) {
            line += tr(" · ✦ new");
        }
        md += line + QLatin1Char('\n');
    }
    flushNoRooms();
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
