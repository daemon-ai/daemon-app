// tst_mirror_entities — the app-side gate for the generated mirror entity layer (spec 09 §3, §12).
//
// This test compiling + linking IS the "generated headers compile in the app build" gate: it pulls
// in the moc-processed Q_GADGET structs, the typed keys, and the provenance table. On top of that
// it asserts:
//   * census completeness — every §3.1 kind is present in the generated EntityKind enum,
//   * provenance completeness — every provenance row is exactly one of wire-sourced | client_local,
//   * canonical key serialization joins composite key components with the \x1f unit separator,
//   * the typed key structs are usable as hash-map keys (immer::table key-fn convention).

#include "mirror/generated/entities_gen.h"
#include "mirror/generated/entities_provenance_gen.h"

#include <QObject>
#include <QSet>
#include <QString>
#include <QTest>
#include <unordered_map>
#include <unordered_set>

using namespace mirror;

namespace {
// The complete §3.1 census (encoded here so a dropped/renamed row fails this gate).
const QSet<QString> kCensus = {"TransportAccount",
                               "Adapter",
                               "Conversation",
                               "ConversationMember",
                               "ChatMessage",
                               "Contact",
                               "Person",
                               "PersonEndpoint",
                               "Session",
                               "FleetUnit",
                               "Approval",
                               "Checkpoint",
                               "Profile",
                               "Credential",
                               "InstalledModel",
                               "ModelDownload",
                               "QuantizeJob",
                               "ProviderDescriptor",
                               "CustomProvider",
                               "RoutePin",
                               "Room",
                               "Notification",
                               "SavedPresence",
                               "ToolInfo",
                               "AgentEntry",
                               "CronJob",
                               "CommandSpec",
                               "RememberedFingerprint",
                               "FsEntry",
                               "TranscriptBlock",
                               "CapsReport",
                               "GatewayStatus",
                               "AccessUser",
                               "RoleInfo"};
} // namespace

class TstMirrorEntities : public QObject {
    Q_OBJECT

private slots:
    void censusIsComplete() {
        QSet<QString> generated;
        for (std::size_t i = 0; i < kEntityKindCount; ++i) {
            generated.insert(QString::fromLatin1(entityKindName(static_cast<EntityKind>(i))));
        }
        QCOMPARE(kEntityKindCount, kCensus.size());
        const QSet<QString> missing = kCensus - generated;
        QVERIFY2(missing.isEmpty(),
                 qPrintable("census rows missing: " + QStringList(missing.values()).join(", ")));
    }

    void everyProvenanceRowIsGrounded() {
        QVERIFY(kProvenanceTableSize > 0);
        for (std::size_t i = 0; i < kProvenanceTableSize; ++i) {
            const FieldProvenance& p = kProvenanceTable[i];
            const bool hasWire = p.wire != nullptr;
            // exactly one of wire-sourced | client_local (spec §3.3).
            QVERIFY2(hasWire != p.client_local,
                     qPrintable(
                         QStringLiteral("%1.%2 provenance is not exactly one of wire|client_local")
                             .arg(p.entity_name, p.field)));
        }
    }

    void everyCensusKindHasProvenance() {
        QSet<QString> withProvenance;
        for (std::size_t i = 0; i < kProvenanceTableSize; ++i) {
            withProvenance.insert(QString::fromLatin1(kProvenanceTable[i].entity_name));
        }
        const QSet<QString> missing = kCensus - withProvenance;
        QVERIFY2(missing.isEmpty(), qPrintable("kinds with no provenance rows: " +
                                               QStringList(missing.values()).join(", ")));
    }

    void compositeKeySerializesWithUnitSeparator() {
        ConversationKey key;
        key.transport = QStringLiteral("matrix-1");
        key.id = QStringLiteral("!room:server");
        const QString sep = QString(QChar(0x1f));
        QCOMPARE(key.serialize(),
                 QStringLiteral("matrix-1") + sep + QStringLiteral("!room:server"));

        // single-component key serializes to the bare component.
        RoomKey rk;
        rk.transport = QStringLiteral("t");
        rk.room = QStringLiteral("r");
        QCOMPARE(rk.serialize(), QStringLiteral("t") + sep + QStringLiteral("r"));
    }

    void keyStructsAreUsableAsHashKeys() {
        // immer::table's key-fn convention: the entity exposes a typed key hashable + comparable.
        std::unordered_map<ConversationKey, int> table;
        Conversation c;
        c.transport = QStringLiteral("matrix-1");
        c.id = QStringLiteral("!a");
        table[c.key()] = 1;

        ConversationKey same{QStringLiteral("matrix-1"), QStringLiteral("!a")};
        QVERIFY(table.find(same) != table.end());
        QCOMPARE(table[same], 1);

        ConversationKey other{QStringLiteral("matrix-1"), QStringLiteral("!b")};
        QVERIFY(table.find(other) == table.end());
        QVERIFY(c.key() == same);
        QVERIFY(c.key() != other);
    }

    void windowedEntityExposesScope() {
        ChatMessage m;
        m.transport = QStringLiteral("matrix-1");
        m.conv = QStringLiteral("!room");
        m.cursor = 42;
        const ChatMessageScope s = m.scope();
        QCOMPARE(s.transport, QStringLiteral("matrix-1"));
        QCOMPARE(s.conv, QStringLiteral("!room"));
        QCOMPARE(ChatMessage::entity_kind, EntityKind::ChatMessage);
    }
};

QTEST_MAIN(TstMirrorEntities)
#include "tst_mirror_entities.moc"
