// tst_mirror_policy_table — the §5.2 event→action policy table (spec 09 §5.2, §12). Asserts the
// table IS the policy: every one of the 16 NodeEvent arms has a declared row (exhaustive over the
// arm census — a new arm without a row fails compilation via policyFor's no-default switch, and
// this runtime test guards the census list), and the declared columns match §5.2.

#include "mirror/policy_table.h"

#include <QObject>
#include <QTest>

using namespace mirror;

class TstMirrorPolicyTable : public QObject {
    Q_OBJECT

private slots:
    void everyArmHasADeclaredRow() {
        // §12: exhaustive over the generated NodeEvent arm census. 16 real arms + Unknown sink.
        const auto& kinds = allNodeEventKinds();
        QCOMPARE(kinds.size(), 17); // 16 §5.2 arms + Unknown
        for (NodeEventKind k : kinds) {
            const PolicyRow row = policyFor(k);
            QCOMPARE(row.kind, k); // the row is the one declared for this arm
        }
    }

    void sessionAdvancedNeverFetches() {
        const PolicyRow p = policyFor(NodeEventKind::SessionAdvanced);
        QCOMPARE(p.action, Action::NudgeOnly);
        QCOMPARE(p.fetchOp, FetchOp::None); // "never a fetch by itself" (§5.2)
    }

    void messagesChangedIsCursorPagedConvHistory() {
        // The rung-0 cursor fix (§13 M1): ConvHistory, no coalesce lane, immediate.
        const PolicyRow p = policyFor(NodeEventKind::MessagesChanged);
        QCOMPARE(p.action, Action::Fetch);
        QCOMPARE(p.fetchOp, FetchOp::ConvHistory);
        QCOMPARE(p.lane, CoalesceLane::None);
    }

    void rosterArmsShareThe300msRosterLane() {
        const PolicyRow roster = policyFor(NodeEventKind::RosterChanged);
        const PolicyRow meta = policyFor(NodeEventKind::SessionMetaChanged);
        QCOMPARE(roster.lane, CoalesceLane::Roster);
        QCOMPARE(meta.lane, CoalesceLane::Roster);
        QCOMPARE(laneDebounceMs(CoalesceLane::Roster), 300);
    }

    void perTransportLanesAre200ms() {
        QCOMPARE(policyFor(NodeEventKind::ConversationsChanged).lane,
                 CoalesceLane::PerTransportConv);
        QCOMPARE(policyFor(NodeEventKind::ContactsChanged).lane, CoalesceLane::PerTransportContact);
        QCOMPARE(laneDebounceMs(CoalesceLane::PerTransportConv), 200);
        QCOMPARE(laneDebounceMs(CoalesceLane::PerTransportContact), 200);
    }

    void conversationsChangedIsKeyedPatch() {
        // SPEC-DECISION: keyed patch (Added→ConvGet, Removed→tombstone), not whole-list refetch.
        QCOMPARE(policyFor(NodeEventKind::ConversationsChanged).action, Action::KeyedPatch);
        QCOMPARE(policyFor(NodeEventKind::ConversationsChanged).fetchOp, FetchOp::ConvGet);
    }

    void payloadCarryingArmsPatchInPlace() {
        QCOMPARE(policyFor(NodeEventKind::DownloadProgress).action, Action::PatchInPlace);
        QCOMPARE(policyFor(NodeEventKind::TransportChanged).action, Action::PatchInPlace);
    }

    void resyncIsMarkStaleScanNeverAStorm() {
        QCOMPARE(policyFor(NodeEventKind::ResyncNeeded).action, Action::MarkStaleScan);
        QCOMPARE(policyFor(NodeEventKind::ResyncNeeded).fetchOp, FetchOp::None);
    }

    void revGatedArmsDeclareTheSkipGate() {
        // §5.2 "full" skip-if-unchanged arms.
        QVERIFY(policyFor(NodeEventKind::FleetChanged).skipIfRevUnchanged);
        QVERIFY(policyFor(NodeEventKind::ProfilesChanged).skipIfRevUnchanged);
        QVERIFY(policyFor(NodeEventKind::CatalogChanged).skipIfRevUnchanged);
        QVERIFY(policyFor(NodeEventKind::NotificationsChanged).skipIfRevUnchanged);
        QVERIFY(policyFor(NodeEventKind::PersonsChanged).skipIfRevUnchanged);
        // Freshness-critical + immediate arms do NOT skip.
        QVERIFY(!policyFor(NodeEventKind::ApprovalPending).skipIfRevUnchanged);
        QVERIFY(!policyFor(NodeEventKind::MessagesChanged).skipIfRevUnchanged);
    }

    void notificationsChangedIsInTheCensusForForwardCompat() {
        // NotificationsChanged is a §5.2/§10.1 arm the v38 app codec cannot yet deliver — it still
        // has a declared row (forward compatibility: a wired arm never lacks a policy).
        const PolicyRow p = policyFor(NodeEventKind::NotificationsChanged);
        QCOMPARE(p.action, Action::Fetch);
        QCOMPARE(p.fetchOp, FetchOp::NotificationList);
    }
};

QTEST_APPLESS_MAIN(TstMirrorPolicyTable)
#include "tst_mirror_policy_table.moc"
