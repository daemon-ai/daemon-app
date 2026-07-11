// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// tst_mock_daemon_parity (mirror A8) — the §13 M5 exit gate: "mock e2e parity suite green (same
// scenarios both modes)". The SAME MockScenario flows through the two feeders:
//
//   MOCK:   the real mock app graph (createAppServiceGraph(Mock)) — the Seeder writes the
//           scenario through the apply pipeline (journal origin = seeder);
//   DAEMON: a daemon-shaped harness — the SAME scenario data delivered through the ingestor's
//           deliver* seams (the exact shape DaemonFetchExecutor hands decoded wire pages to),
//           plus a TransportChanged ingest for the account (the daemon transport-account path).
//
// Then the SHARED VM layer must render equivalently: the mirror tables (value parity where the
// data carries no wall-clock), the ConversationListModel / ChatWindowModel lenses (what GUI+TUI
// bind), and the MirrorSessionStore projection (roster/sidebar/pickers). §9 "same store, same
// journal, same lenses, different feeder" — parity by construction, asserted.

#include "daemon/app_service_graph.h"
#include "daemon/mirror_session_store.h"
#include "daemon/mock_scenario.h"
#include "mirror/chat_window_model.h"
#include "mirror/conversation_list_model.h"
#include "mirror/mirror_service.h"
#include "mirror/parity.h"

#include <QHash>
#include <QSet>
#include <QtTest>
#include <vector>

using daemonapp::daemon::MockScenario;
using daemonapp::daemon::mockScenarioByName;

namespace {

// Deliver one MockScenario through the ingestor's deliver* seams — the daemon-mode shape (what
// the DaemonFetchExecutor delivers after decoding wire pages), grouped per transport exactly like
// the per-transport wire reads.
void deliverScenarioAsDaemon(mirror::MirrorService& svc, const MockScenario& scn) {
    const mirror::SeedSet& seed = scn.mirror.seed;

    QHash<QString, std::vector<mirror::Conversation>> convsByTransport;
    for (const mirror::Conversation& c : seed.conversations) {
        convsByTransport[c.transport].push_back(c);
    }
    for (auto it = convsByTransport.constBegin(); it != convsByTransport.constEnd(); ++it) {
        svc.ingestor().deliverConversations(it.key(), it.value(), /*isFinalPage=*/true);
    }

    QHash<QString, std::vector<mirror::Contact>> contactsByTransport;
    for (const mirror::Contact& c : seed.contacts) {
        contactsByTransport[c.transport].push_back(c);
    }
    for (auto it = contactsByTransport.constBegin(); it != contactsByTransport.constEnd(); ++it) {
        svc.ingestor().deliverContacts(it.key(), it.value(), /*isFinalPage=*/true);
    }

    svc.ingestor().deliverPersons(seed.persons, /*isFinalPage=*/true);
    svc.ingestor().deliverSessions(seed.sessions, /*isFinalPage=*/true);
    svc.ingestor().deliverFleetUnits(seed.fleetUnits, /*isFinalPage=*/true);
    svc.ingestor().deliverRoutePins(seed.routePins, /*isFinalPage=*/true);

    QHash<QString, std::vector<mirror::Room>> roomsByTransport;
    for (const mirror::Room& r : seed.rooms) {
        roomsByTransport[r.transport].push_back(r);
    }
    for (auto it = roomsByTransport.constBegin(); it != roomsByTransport.constEnd(); ++it) {
        svc.ingestor().deliverRooms(it.key(), it.value(), /*isFinalPage=*/true);
    }

    for (auto it = seed.chat.constBegin(); it != seed.chat.constEnd(); ++it) {
        const std::vector<mirror::ChatMessage>& msgs = it.value();
        if (msgs.empty()) {
            continue;
        }
        const quint64 head = msgs.back().cursor;
        svc.ingestor().deliverChatPage(msgs.front().transport, msgs.front().conv, msgs, head, head);
    }

    // Transcript blocks land through the ingestor's cursor-ordered (session, seq) upsert — the
    // exact deliver seam the engine dual-write / journal replay drives (A7T/G2).
    for (const mirror::TranscriptBlock& tb : seed.transcriptBlocks) {
        svc.ingestor().deliverTranscriptBlock(tb);
    }

    // Transport accounts land via the TransportChanged patch in daemon mode (§5.2 — the event
    // carries liveness fields only; family/profile join with the transports vertical post-M5).
    for (const mirror::TransportAccount& a : seed.transportAccounts) {
        mirror::NodeEvent e;
        e.kind = mirror::NodeEventKind::TransportChanged;
        e.transport = a.transport;
        e.connection = a.connection;
        e.presence = a.presence;
        e.hasPresence = true;
        svc.ingest(e);
    }
}

// Session identity fields (last_activity_ms is wall-clock-stamped per defaultSeed() call, so two
// scenario instances legitimately differ there; everything the surfaces render must agree).
QString sessionFingerprint(const mirror::Session& s) {
    return s.session + QChar(0x1f) + s.title + QChar(0x1f) + s.state + QChar(0x1f) + s.lifecycle +
           QChar(0x1f) + s.role + QChar(0x1f) + s.bound_profile + QChar(0x1f) +
           (s.pinned ? QLatin1String("1") : QLatin1String("0")) + QChar(0x1f) +
           (s.archived ? QLatin1String("1") : QLatin1String("0"));
}

template <typename Model>
QStringList lensColumn(Model& model, int role) {
    QStringList out;
    out.reserve(model.rowCount());
    for (int i = 0; i < model.rowCount(); ++i) {
        out << model.data(model.index(i, 0), role).toString();
    }
    return out;
}

} // namespace

class TstMockDaemonParity : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { qunsetenv("DAEMON_APP_MOCK_SCENARIO"); }

    void sameScenarioRendersEquivalentlyInBothModes() {
        // MOCK side: the real app graph (seeder-fed at construction).
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);
        QVERIFY(graph.mirrorService != nullptr);
        const mirror::MirrorModel& mockSnap = graph.mirrorService->store().snapshot();

        // DAEMON side: the same scenario through the ingestor deliver seams.
        const MockScenario scn = mockScenarioByName(QStringLiteral("default"));
        mirror::MirrorService daemonSvc;
        daemonSvc.openInMemory();
        deliverScenarioAsDaemon(daemonSvc, scn);
        const mirror::MirrorModel& daemonSnap = daemonSvc.store().snapshot();

        // --- table parity (canonical keys via the shared parity helpers; §13 gate form) -------
        const QString t = QStringLiteral("matrix/@you:matrix.org");
        QVERIFY(mirror::parity::compareKeys(mirror::parity::conversationKeys(mockSnap, t),
                                            mirror::parity::conversationKeys(daemonSnap, t))
                    .matches());
        QVERIFY(mirror::parity::compareKeys(mirror::parity::contactKeys(mockSnap, t),
                                            mirror::parity::contactKeys(daemonSnap, t))
                    .matches());
        QVERIFY(mirror::parity::compareKeys(mirror::parity::sessionKeys(mockSnap),
                                            mirror::parity::sessionKeys(daemonSnap))
                    .matches());
        QVERIFY(mirror::parity::compareKeys(mirror::parity::fleetUnitKeys(mockSnap),
                                            mirror::parity::fleetUnitKeys(daemonSnap))
                    .matches());

        // Value parity where the entities carry no wall-clock: conversations, route pins, rooms.
        for (const mirror::Conversation& c : mockSnap.conversations) {
            const mirror::Conversation* d = daemonSnap.conversations.find(c.key());
            QVERIFY(d != nullptr);
            QVERIFY(*d == c);
        }
        for (const mirror::RoutePin& p : mockSnap.route_pins) {
            const mirror::RoutePin* d = daemonSnap.route_pins.find(p.key());
            QVERIFY(d != nullptr);
            QVERIFY(*d == p);
        }
        for (const mirror::Room& r : mockSnap.rooms) {
            const mirror::Room* d = daemonSnap.rooms.find(r.key());
            QVERIFY(d != nullptr);
            QVERIFY(*d == r);
        }
        // Transport accounts: KEY parity (the daemon event patch carries liveness fields only).
        QSet<QString> mockAccounts;
        for (const mirror::TransportAccount& a : mockSnap.transport_accounts) {
            mockAccounts.insert(a.key().serialize());
        }
        QSet<QString> daemonAccounts;
        for (const mirror::TransportAccount& a : daemonSnap.transport_accounts) {
            daemonAccounts.insert(a.key().serialize());
        }
        QCOMPARE(daemonAccounts, mockAccounts);

        // Session identity parity (wall-clock excluded — the surfaces render these fields).
        QSet<QString> mockSessions;
        for (const mirror::Session& s : mockSnap.sessions) {
            mockSessions.insert(sessionFingerprint(s));
        }
        QSet<QString> daemonSessions;
        for (const mirror::Session& s : daemonSnap.sessions) {
            daemonSessions.insert(sessionFingerprint(s));
        }
        QCOMPARE(daemonSessions, mockSessions);

        // --- lens parity: the models GUI + TUI actually bind render the same rows -------------
        mirror::ConversationListModel mockConvs(graph.mirrorService->store());
        mirror::ConversationListModel daemonConvs(daemonSvc.store());
        QCOMPARE(mockConvs.rowCount(), daemonConvs.rowCount());
        QCOMPARE(lensColumn(mockConvs, mirror::ConversationListModel::TitleRole),
                 lensColumn(daemonConvs, mirror::ConversationListModel::TitleRole));

        const mirror::ChatMessageScope scope{t, QStringLiteral("!daemon-dev:matrix.org")};
        mirror::ChatWindowModel mockChat(graph.mirrorService->store(), scope);
        mirror::ChatWindowModel daemonChat(daemonSvc.store(), scope);
        QCOMPARE(mockChat.rowCount(), daemonChat.rowCount());
        QCOMPARE(lensColumn(mockChat, mirror::ChatWindowModel::TextRole),
                 lensColumn(daemonChat, mirror::ChatWindowModel::TextRole));
        QCOMPARE(lensColumn(mockChat, mirror::ChatWindowModel::AuthorRole),
                 lensColumn(daemonChat, mirror::ChatWindowModel::AuthorRole));

        // --- store-projection parity: the 6→1 session read both modes' consumers bind ---------
        daemonapp::daemon::MirrorSessionStore daemonStore(&daemonSvc.store(),
                                                          &daemonSvc.ingestor());
        const QList<domain::Session> mockRows = graph.storeMirror->sessions(domain::ListScope{});
        const QList<domain::Session> daemonRows = daemonStore.sessions(domain::ListScope{});
        QCOMPARE(mockRows.size(), daemonRows.size());
        QSet<QString> mockRowIds;
        for (const domain::Session& s : mockRows) {
            mockRowIds.insert(s.sessionId.toString() + QChar(0x1f) + s.title);
        }
        QSet<QString> daemonRowIds;
        for (const domain::Session& s : daemonRows) {
            daemonRowIds.insert(s.sessionId.toString() + QChar(0x1f) + s.title);
        }
        QCOMPARE(daemonRowIds, mockRowIds);

        // --- transcript CONTENT parity: the msg-fence projection both feeders produce agrees
        // (mirror-vs-mirror — the legacy baselines died with the legacy stores, AD) -------------
        const domain::SessionId scratch{QStringLiteral("s-scratch")};
        const QString mockContent = graph.storeMirror->content(scratch);
        QVERIFY(!mockContent.isEmpty());
        QCOMPARE(daemonStore.content(scratch), mockContent); // mirror, both feeders agree
    }

    // The journal origins differ BY DESIGN (seeder vs refetch_diff) while the rendered state is
    // identical — the §4.3 origin column records the feeder; nothing downstream forks on it.
    void originsRecordTheFeederNotTheShape() {
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);
        for (const mirror::JournalRecord& rec : graph.mirrorService->store().journal().records()) {
            QCOMPARE(rec.origin, mirror::JournalOrigin::Seeder);
        }

        const MockScenario scn = mockScenarioByName(QStringLiteral("default"));
        mirror::MirrorService daemonSvc;
        daemonSvc.openInMemory();
        deliverScenarioAsDaemon(daemonSvc, scn);
        for (const mirror::JournalRecord& rec : daemonSvc.store().journal().records()) {
            QVERIFY(rec.origin != mirror::JournalOrigin::Seeder);
        }
    }
};

QTEST_GUILESS_MAIN(TstMockDaemonParity)
#include "tst_mock_daemon_parity.moc"
