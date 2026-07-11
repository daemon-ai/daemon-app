// tst_mirror_bridge — the daemon bridge translation (spec 09 §5.1, §12). Asserts DecodedNodeEvent
// (the app's generated wire NodeEvent surface) maps correctly to mirror::NodeEvent, and that the
// translation preserves the per-arm fields the policy table + ingestor read. The translate()
// switch's compile-time exhaustiveness (no default) is the second layer tying the wire enum to the
// policy table; this test guards the field-copy behavior.

#include "daemon/ingestor_bridge.h"
#include "mirror/policy_table.h"

#include <QObject>
#include <QTest>

using namespace daemonapp::daemon;

class TstMirrorBridge : public QObject {
    Q_OBJECT

private slots:
    void messagesChangedCarriesTransportAndConv() {
        DecodedNodeEvent in;
        in.kind = DecodedNodeEvent::Kind::MessagesChanged;
        in.transport = QStringLiteral("matrix-1");
        in.conv = QStringLiteral("!room");
        const mirror::NodeEvent out = translateNodeEvent(in);
        QCOMPARE(out.kind, mirror::NodeEventKind::MessagesChanged);
        QCOMPARE(out.transport, QStringLiteral("matrix-1"));
        QCOMPARE(out.conv, QStringLiteral("!room"));
    }

    void resyncCarriesScope() {
        DecodedNodeEvent in;
        in.kind = DecodedNodeEvent::Kind::ResyncNeeded;
        in.scope = QStringLiteral("profiles");
        const mirror::NodeEvent out = translateNodeEvent(in);
        QCOMPARE(out.kind, mirror::NodeEventKind::ResyncNeeded);
        QCOMPARE(out.scope, QStringLiteral("profiles"));
    }

    // AD (§10.3 carrier 3): the single-mutation event arms carry the causing op's client op_id;
    // the translate preserves it so the ingestor threads it into the triggered fetch and the
    // resulting apply lands the outbox op (§6.6).
    void sessionMetaChangedCarriesOriginOp() {
        DecodedNodeEvent in;
        in.kind = DecodedNodeEvent::Kind::SessionMetaChanged;
        in.session = QStringLiteral("s-1");
        in.rev = 7;
        in.originOp = QStringLiteral("op-abc");
        const mirror::NodeEvent out = translateNodeEvent(in);
        QCOMPARE(out.kind, mirror::NodeEventKind::SessionMetaChanged);
        QCOMPARE(out.session, QStringLiteral("s-1"));
        QCOMPARE(out.originOp, QStringLiteral("op-abc"));

        DecodedNodeEvent conv;
        conv.kind = DecodedNodeEvent::Kind::ConversationsChanged;
        conv.transport = QStringLiteral("m");
        conv.conv = QStringLiteral("!c");
        conv.convChange = QStringLiteral("removed");
        conv.originOp = QStringLiteral("op-def");
        QCOMPARE(translateNodeEvent(conv).originOp, QStringLiteral("op-def"));
    }

    void membershipSelfRemovalPreserved() {
        DecodedNodeEvent in;
        in.kind = DecodedNodeEvent::Kind::MembershipChanged;
        in.transport = QStringLiteral("m");
        in.conv = QStringLiteral("!c");
        in.membershipChange = QStringLiteral("kicked");
        in.isSelf = true;
        const mirror::NodeEvent out = translateNodeEvent(in);
        QCOMPARE(out.kind, mirror::NodeEventKind::MembershipChanged);
        QVERIFY(out.isSelf);
        QCOMPARE(out.membershipChange, QStringLiteral("kicked"));
    }

    void transportChangedCarriesConnectionState() {
        DecodedNodeEvent in;
        in.kind = DecodedNodeEvent::Kind::TransportChanged;
        in.transport = QStringLiteral("m");
        in.connection = QStringLiteral("connected");
        in.presence = QStringLiteral("available");
        in.hasPresence = true;
        const mirror::NodeEvent out = translateNodeEvent(in);
        QCOMPARE(out.kind, mirror::NodeEventKind::TransportChanged);
        QCOMPARE(out.connection, QStringLiteral("connected"));
        QVERIFY(out.hasPresence);
    }

    void downloadProgressCarriesPayload() {
        DecodedNodeEvent in;
        in.kind = DecodedNodeEvent::Kind::DownloadProgress;
        in.downloadId = 9;
        in.state = QStringLiteral("downloading");
        in.downloadedBytes = 100;
        in.totalBytes = 200;
        const mirror::NodeEvent out = translateNodeEvent(in);
        QCOMPARE(out.kind, mirror::NodeEventKind::DownloadProgress);
        QCOMPARE(out.downloadId, quint64(9));
        QCOMPARE(out.downloadedBytes, quint64(100));
    }

    void everyWireArmTranslatesToAPolicyBackedKind() {
        // Each generated wire arm must translate to a mirror arm that has a declared policy row.
        const DecodedNodeEvent::Kind kinds[] = {
            DecodedNodeEvent::Kind::Unknown,
            DecodedNodeEvent::Kind::SessionAdvanced,
            DecodedNodeEvent::Kind::SessionMetaChanged,
            DecodedNodeEvent::Kind::RosterChanged,
            DecodedNodeEvent::Kind::ApprovalPending,
            DecodedNodeEvent::Kind::DownloadProgress,
            DecodedNodeEvent::Kind::CatalogChanged,
            DecodedNodeEvent::Kind::ResyncNeeded,
            DecodedNodeEvent::Kind::FleetChanged,
            DecodedNodeEvent::Kind::TransportChanged,
            DecodedNodeEvent::Kind::ConversationsChanged,
            DecodedNodeEvent::Kind::MembershipChanged,
            DecodedNodeEvent::Kind::ContactsChanged,
            DecodedNodeEvent::Kind::ProfilesChanged,
            DecodedNodeEvent::Kind::PersonsChanged,
            DecodedNodeEvent::Kind::MessagesChanged,
        };
        for (DecodedNodeEvent::Kind k : kinds) {
            DecodedNodeEvent in;
            in.kind = k;
            const mirror::NodeEvent out = translateNodeEvent(in);
            const mirror::PolicyRow row = mirror::policyFor(out.kind);
            QCOMPARE(row.kind, out.kind); // a declared policy row exists for the translated arm
        }
    }
};

QTEST_APPLESS_MAIN(TstMirrorBridge)
#include "tst_mirror_bridge.moc"
