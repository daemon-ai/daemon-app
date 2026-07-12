// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// tst_mock_scenario (mirror A8, M5) — the mock scenario catalog (spec 09 §9). Asserts:
//  (1) the default scenario's mirror session/fleet rows are DERIVED from the one SeedBundle —
//      id sets agree exactly (the delegated content() join depends on this);
//  (2) derived rows speak the canonical string vocabulary (entities_map.cpp's enum strings, the
//      same ones MirrorSessionStore::projectRow consumes);
//  (3) chat windows are seeded under conversation scopes that exist in the conversation table;
//  (4) the verb script carries the MANDATORY rejection rule (§9) + a timeout rule and defaults
//      unmatched sends to ok-with-echo (§6.6 provenance echo);
//  (5) the api38 variant differs ONLY in the advertised api/<N>; the empty variant seeds nothing;
//  (6) DAEMON_APP_MOCK_SCENARIO selects by name; unknown names fall back to default.

#include "daemon/mock_scenario.h"

#include <QJsonObject>
#include <QSet>
#include <QtTest>

using daemonapp::daemon::MockScenario;
using daemonapp::daemon::mockScenarioByName;
using daemonapp::daemon::mockScenarioFromEnvironment;

namespace {

QSet<QString> bundleSessionIds(const MockScenario& s) {
    QSet<QString> ids;
    for (const domain::Session& d : s.bundle.sessions) {
        ids.insert(d.sessionId.toString());
    }
    return ids;
}

QSet<QString> mirrorSessionIds(const MockScenario& s) {
    QSet<QString> ids;
    for (const mirror::Session& m : s.mirror.seed.sessions) {
        ids.insert(m.session);
    }
    return ids;
}

} // namespace

class TstMockScenario : public QObject {
    Q_OBJECT

private slots:
    void init() { qunsetenv("DAEMON_APP_MOCK_SCENARIO"); }
    void cleanupTestCase() { qunsetenv("DAEMON_APP_MOCK_SCENARIO"); }

    void defaultDerivesMirrorRowsFromBundle() {
        const MockScenario s = mockScenarioByName(QStringLiteral("default"));
        QVERIFY(!s.bundle.sessions.isEmpty());
        QVERIFY(!s.bundle.units.isEmpty());
        QCOMPARE(mirrorSessionIds(s), bundleSessionIds(s)); // id agreement, both directions

        QSet<QString> bundleUnits;
        for (const domain::UnitNode& u : s.bundle.units) {
            bundleUnits.insert(u.id.toString());
        }
        QSet<QString> mirrorUnits;
        for (const mirror::FleetUnit& f : s.mirror.seed.fleetUnits) {
            mirrorUnits.insert(f.id);
        }
        QCOMPARE(mirrorUnits, bundleUnits);
    }

    void derivedRowsSpeakCanonicalVocabulary() {
        const MockScenario s = mockScenarioByName(QStringLiteral("default"));
        const QSet<QString> states = {QStringLiteral("Active"), QStringLiteral("Suspended"),
                                      QStringLiteral("Ready"), QStringLiteral("Completed"),
                                      QStringLiteral("Unknown")};
        const QSet<QString> lifecycles = {QStringLiteral("Live"), QStringLiteral("Durable")};
        const QSet<QString> roles = {QStringLiteral("Primary"), QStringLiteral("ManagedChild"),
                                     QStringLiteral("EphemeralSubagent")};
        for (const mirror::Session& m : s.mirror.seed.sessions) {
            QVERIFY2(states.contains(m.state), qPrintable(m.state));
            QVERIFY2(lifecycles.contains(m.lifecycle), qPrintable(m.lifecycle));
            QVERIFY2(roles.contains(m.role), qPrintable(m.role));
        }
        const QSet<QString> kinds = {QStringLiteral("Engine"), QStringLiteral("Orchestrator"),
                                     QStringLiteral("Host")};
        const QSet<QString> unitStates = {QStringLiteral("Running"), QStringLiteral("Finished"),
                                          QStringLiteral("Unknown")};
        for (const mirror::FleetUnit& f : s.mirror.seed.fleetUnits) {
            QVERIFY2(kinds.contains(f.kind), qPrintable(f.kind));
            QVERIFY2(unitStates.contains(f.state), qPrintable(f.state));
        }
    }

    void chatWindowsScopeSeededConversations() {
        const MockScenario s = mockScenarioByName(QStringLiteral("default"));
        QVERIFY(!s.mirror.seed.conversations.empty());
        QVERIFY(!s.mirror.seed.chat.isEmpty());
        QSet<QString> convScopes;
        for (const mirror::Conversation& c : s.mirror.seed.conversations) {
            convScopes.insert(c.transport + QChar(0x1f) + c.id);
        }
        for (auto it = s.mirror.seed.chat.constBegin(); it != s.mirror.seed.chat.constEnd(); ++it) {
            QVERIFY2(convScopes.contains(it.key()), qPrintable(it.key()));
            QVERIFY(!it.value().empty());
            // Cursor-ordered, strictly increasing (the window key).
            quint64 prev = 0;
            for (const mirror::ChatMessage& m : it.value()) {
                QVERIFY(m.cursor > prev);
                prev = m.cursor;
            }
        }
        // Routing demo present: the routing manager renders non-empty in mock (the A6 gap).
        QVERIFY(!s.mirror.seed.routePins.empty());
        QVERIFY(!s.mirror.seed.rooms.empty());
        QVERIFY(!s.mirror.seed.transportAccounts.empty());
    }

    void verbScriptCarriesMandatoryRejectionRules() {
        const MockScenario s = mockScenarioByName(QStringLiteral("default"));
        QJsonObject reject{{QStringLiteral("message"), QStringLiteral("/reject please")}};
        QJsonObject timeout{{QStringLiteral("message"), QStringLiteral("/timeout now")}};
        QJsonObject plain{{QStringLiteral("message"), QStringLiteral("hello there")}};

        const mirror::VerbOutcome r =
            s.mirror.verbScript.outcomeFor(QStringLiteral("ConvSend"), reject);
        QCOMPARE(r.kind, mirror::VerbOutcomeKind::Reject);
        QCOMPARE(r.errorKind, QStringLiteral("Forbidden"));

        const mirror::VerbOutcome t =
            s.mirror.verbScript.outcomeFor(QStringLiteral("ConvSend"), timeout);
        QCOMPARE(t.kind, mirror::VerbOutcomeKind::Timeout);

        const mirror::VerbOutcome ok =
            s.mirror.verbScript.outcomeFor(QStringLiteral("ConvSend"), plain);
        QCOMPARE(ok.kind, mirror::VerbOutcomeKind::Ok);
        QVERIFY(ok.echoDelayMs > 0); // the §6.6 provenance echo is scripted
    }

    void api38VariantDiffersOnlyInApiVersion() {
        const MockScenario def = mockScenarioByName(QStringLiteral("default"));
        const MockScenario v38 = mockScenarioByName(QStringLiteral("api38"));
        QCOMPARE(def.mirror.apiVersion, 39);
        QCOMPARE(v38.mirror.apiVersion, 38);
        QCOMPARE(mirrorSessionIds(v38), mirrorSessionIds(def)); // same data
        QCOMPARE(v38.mirror.seed.chat.size(), def.mirror.seed.chat.size());
    }

    void emptyScenarioSeedsNothing() {
        const MockScenario s = mockScenarioByName(QStringLiteral("empty"));
        QVERIFY(s.bundle.sessions.isEmpty());
        QVERIFY(s.mirror.seed.sessions.empty());
        QVERIFY(s.mirror.seed.conversations.empty());
        QVERIFY(s.mirror.seed.chat.isEmpty());
        QCOMPARE(s.mirror.apiVersion, 39);
    }

    void environmentSelectsAndFallsBack() {
        qputenv("DAEMON_APP_MOCK_SCENARIO", "api38");
        QCOMPARE(mockScenarioFromEnvironment().name, QStringLiteral("api38"));
        qputenv("DAEMON_APP_MOCK_SCENARIO", "no-such-scenario");
        QCOMPARE(mockScenarioFromEnvironment().name, QStringLiteral("default"));
        qunsetenv("DAEMON_APP_MOCK_SCENARIO");
        QCOMPARE(mockScenarioFromEnvironment().name, QStringLiteral("default"));
    }
};

QTEST_GUILESS_MAIN(TstMockScenario)
#include "tst_mock_scenario.moc"
