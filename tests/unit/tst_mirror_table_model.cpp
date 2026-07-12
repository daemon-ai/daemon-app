// tst_mirror_table_model — the generic journal-delta adapter mirror::TableModel<Entity>
// (spec 09 §8.1/§8.3; §14.8). Every model instantiation runs under QAbstractItemModelTester
// (§12). Covers: exact row ops vs journal deltas (insert/remove/dataChanged/move), sorted
// projection, NO reset after initial population, 64-bit stable-id stability, the auto Q_GADGET
// role map, the curated ConversationListModel demonstrator, and a property-style test (random
// delta sequences ⇒ the model mirrors the table, no resets).
//
// The incremental-model-update behavior and the delta→row-op dispatch are re-expressed (behaviors
// only; Sink is LGPL) from Sink's live add/remove reaching the model
// (references/architecture/sink/tests/querytest.cpp:903-943) and its delta dispatch precedent
// (common/queryrunner.cpp:294-341).

#include "mirror/conversation_list_model.h"
#include "mirror/observe_coarse.h"
#include "mirror/store.h"
#include "mirror/table_model.h"

#include <memory>
#include <QAbstractItemModelTester>
#include <QSet>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <random>
#include <vector>

using namespace mirror;

namespace {
Conversation conv(const QString& transport, const QString& id, const QString& title) {
    Conversation c;
    c.transport = transport;
    c.id = id;
    c.title = title;
    return c;
}

int roleFor(const QAbstractItemModel& m, const QByteArray& name) {
    const auto names = m.roleNames();
    for (auto it = names.constBegin(); it != names.constEnd(); ++it) {
        if (it.value() == name) {
            return it.key();
        }
    }
    return -1;
}

QString keyAtRow(const QAbstractItemModel& m, int row) {
    const int tRole = roleFor(m, "transport");
    const int iRole = roleFor(m, "id");
    return m.data(m.index(row, 0), tRole).toString() + QChar(0x1f) +
           m.data(m.index(row, 0), iRole).toString();
}
} // namespace

class TstMirrorTableModel : public QObject {
    Q_OBJECT

private slots:
    void initialPopulationFromSnapshotIsSortedNoReset() {
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!c"), QStringLiteral("C")))
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A")))
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!b"), QStringLiteral("B")))
            .commit();

        TableModel<Conversation> model(store);
        QAbstractItemModelTester tester(&model);
        QSignalSpy resetSpy(&model, &QAbstractItemModel::modelAboutToBeReset);

        QCOMPARE(model.rowCount(), 3);
        // Default sort = identity (serialized key): !a, !b, !c.
        QCOMPARE(keyAtRow(model, 0), QStringLiteral("m\x1f!a"));
        QCOMPARE(keyAtRow(model, 1), QStringLiteral("m\x1f!b"));
        QCOMPARE(keyAtRow(model, 2), QStringLiteral("m\x1f!c"));
        QCOMPARE(resetSpy.count(), 0); // initial population never emits a reset (§14.8)
    }

    void upsertOfNewKeyInsertsRowAtSortedPosition() {
        CoarseObserve obs;
        Store store(obs);
        TableModel<Conversation> model(store);
        QAbstractItemModelTester tester(&model);
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        QSignalSpy resetSpy(&model, &QAbstractItemModel::modelAboutToBeReset);

        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!b"), QStringLiteral("B")))
            .commit();
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A")))
            .commit();

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(inserted.count(), 2);
        QCOMPARE(keyAtRow(model, 0), QStringLiteral("m\x1f!a")); // inserted BEFORE !b
        QCOMPARE(keyAtRow(model, 1), QStringLiteral("m\x1f!b"));
        QCOMPARE(resetSpy.count(), 0);
    }

    void tombstoneRemovesExactRow() {
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A")))
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!b"), QStringLiteral("B")))
            .commit();
        TableModel<Conversation> model(store);
        QAbstractItemModelTester tester(&model);
        QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);

        store.beginBatch()
            .tombstone<Conversation>(ConversationKey{QStringLiteral("m"), QStringLiteral("!a")})
            .commit();

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(removed.count(), 1);
        QCOMPARE(keyAtRow(model, 0), QStringLiteral("m\x1f!b"));
    }

    void inPlaceUpdateEmitsDataChangedNotReset() {
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A")))
            .commit();
        TableModel<Conversation> model(store);
        QAbstractItemModelTester tester(&model);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
        QSignalSpy moved(&model, &QAbstractItemModel::rowsMoved);

        // A title change does NOT move under the default (identity) sort — same key, same pos.
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A-edited")))
            .commit();

        QCOMPARE(changed.count(), 1);
        QCOMPARE(moved.count(), 0);
        QCOMPARE(model.data(model.index(0, 0), roleFor(model, "title")).toString(),
                 QStringLiteral("A-edited"));
    }

    void sortKeyChangeMovesRow() {
        // ConversationListModel sorts by title; a rename that reorders emits an exact move,
        // never a reset (§14.8).
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!1"), QStringLiteral("Alpha")))
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!2"), QStringLiteral("Bravo")))
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!3"), QStringLiteral("Charlie")))
            .commit();
        ConversationListModel model(store);
        QAbstractItemModelTester tester(&model);
        QSignalSpy moved(&model, &QAbstractItemModel::rowsMoved);
        QSignalSpy resetSpy(&model, &QAbstractItemModel::modelAboutToBeReset);

        const int titleRole = roleFor(model, "title");
        QCOMPARE(model.data(model.index(0, 0), titleRole).toString(), QStringLiteral("Alpha"));

        // Rename "Alpha" -> "Zulu": it must move to the end.
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!1"), QStringLiteral("Zulu")))
            .commit();

        QCOMPARE(moved.count(), 1);
        QCOMPARE(resetSpy.count(), 0);
        QCOMPARE(model.data(model.index(0, 0), titleRole).toString(), QStringLiteral("Bravo"));
        QCOMPARE(model.data(model.index(1, 0), titleRole).toString(), QStringLiteral("Charlie"));
        QCOMPARE(model.data(model.index(2, 0), titleRole).toString(), QStringLiteral("Zulu"));
    }

    void stableIdSurvivesUpdatesReordersAndReadd() {
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!1"), QStringLiteral("Alpha")))
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!2"), QStringLiteral("Bravo")))
            .commit();
        ConversationListModel model(store);
        QAbstractItemModelTester tester(&model);

        const qint64 alphaId = model.stableIdAt(0); // Alpha at row 0
        QVERIFY(alphaId != 0);
        QCOMPARE(model.rowForStableId(alphaId), 0);

        // Update Alpha's data (no reorder): id unchanged.
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!1"), QStringLiteral("Alpha!")))
            .commit();
        QCOMPARE(model.stableIdAt(0), alphaId);

        // Reorder Alpha to the end (rename): id follows the row, not the index.
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!1"), QStringLiteral("Zulu")))
            .commit();
        QCOMPARE(model.rowForStableId(alphaId), 1); // now last
        QCOMPARE(model.stableIdAt(1), alphaId);

        // Remove then re-add the same key: interning is retained, so the id is unchanged.
        store.beginBatch()
            .tombstone<Conversation>(ConversationKey{QStringLiteral("m"), QStringLiteral("!1")})
            .commit();
        QCOMPARE(model.rowForStableId(alphaId), -1);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!1"), QStringLiteral("Zulu")))
            .commit();
        QCOMPARE(model.stableIdAt(model.rowForStableId(alphaId)), alphaId);
    }

    void stableIdsAreDistinctAcrossRows() {
        CoarseObserve obs;
        Store store(obs);
        auto b = store.beginBatch();
        for (int i = 0; i < 20; ++i) {
            b.upsert(conv(QStringLiteral("m"), QStringLiteral("!%1").arg(i),
                          QStringLiteral("t%1").arg(i)));
        }
        b.commit();
        TableModel<Conversation> model(store);
        QAbstractItemModelTester tester(&model);

        QSet<qint64> ids;
        for (int r = 0; r < model.rowCount(); ++r) {
            ids.insert(model.stableIdAt(r));
        }
        QCOMPARE(ids.size(), 20);
        QVERIFY(!ids.contains(0));
    }

    void autoRoleMapExposesGadgetProperties() {
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("Title")))
            .commit();
        TableModel<Conversation> model(store);
        QAbstractItemModelTester tester(&model);

        QVERIFY(roleFor(model, "stableId") != -1);
        QVERIFY(roleFor(model, "title") != -1);
        QVERIFY(roleFor(model, "transport") != -1);
        QCOMPARE(model.data(model.index(0, 0), roleFor(model, "title")).toString(),
                 QStringLiteral("Title"));
    }

    void curatedRoleMapDemonstrator() {
        CoarseObserve obs;
        Store store(obs);
        Conversation c = conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("Room"));
        c.topic = QStringLiteral("Topic");
        c.member_count = 7;
        store.beginBatch().upsert(c).commit();
        ConversationListModel model(store);
        QAbstractItemModelTester tester(&model);

        const auto names = model.roleNames();
        QVERIFY(names.values().contains(QByteArrayLiteral("stableId")));
        QVERIFY(names.values().contains(QByteArrayLiteral("conversationId")));
        QVERIFY(names.values().contains(QByteArrayLiteral("memberCount")));
        // Curated map: no raw "id"/"description" property roles leak through.
        QCOMPARE(roleFor(model, "description"), -1);

        QCOMPARE(model.data(model.index(0, 0), roleFor(model, "conversationId")).toString(),
                 QStringLiteral("!a"));
        QCOMPARE(model.data(model.index(0, 0), roleFor(model, "memberCount")).toInt(), 7);
        QCOMPARE(model.data(model.index(0, 0), roleFor(model, "topic")).toString(),
                 QStringLiteral("Topic"));
    }

    void propertyRandomDeltasMirrorTableWithoutResets() {
        // Random upsert/tombstone sequences over a small key space; after every commit the model
        // must exactly mirror the store table (sorted keys + field values). Tester attached
        // throughout catches any invalid row op; the reset counter must stay 0.
        CoarseObserve obs;
        Store store(obs);
        TableModel<Conversation> model(store);
        QAbstractItemModelTester tester(&model);
        QSignalSpy resetSpy(&model, &QAbstractItemModel::modelAboutToBeReset);

        std::mt19937 rng(0xA3C0FFEE);
        std::uniform_int_distribution<int> keyDist(0, 7);
        std::uniform_int_distribution<int> opDist(0, 2); // 0,1 upsert; 2 tombstone
        std::uniform_int_distribution<int> verDist(0, 999);

        for (int step = 0; step < 400; ++step) {
            auto batch = store.beginBatch();
            const int ops = 1 + (step % 4);
            for (int k = 0; k < ops; ++k) {
                const QString id = QStringLiteral("!%1").arg(keyDist(rng));
                if (opDist(rng) == 2) {
                    batch.tombstone<Conversation>(ConversationKey{QStringLiteral("m"), id});
                } else {
                    batch.upsert(
                        conv(QStringLiteral("m"), id, QStringLiteral("v%1").arg(verDist(rng))));
                }
            }
            batch.commit();

            // Expected = table contents sorted by identity (default TableModel order).
            std::vector<Conversation> expected;
            for (const Conversation& c : store.snapshot().conversations) {
                expected.push_back(c);
            }
            std::sort(expected.begin(), expected.end(),
                      [](const Conversation& a, const Conversation& b) {
                          return a.key().serialize() < b.key().serialize();
                      });
            QCOMPARE(model.rowCount(), static_cast<int>(expected.size()));
            for (int r = 0; r < model.rowCount(); ++r) {
                QCOMPARE(keyAtRow(model, r),
                         expected[static_cast<std::size_t>(r)].key().serialize());
                QCOMPARE(model.data(model.index(r, 0), roleFor(model, "title")).toString(),
                         expected[static_cast<std::size_t>(r)].title);
            }
        }
        QCOMPARE(resetSpy.count(), 0);
    }
};

QTEST_MAIN(TstMirrorTableModel)
#include "tst_mirror_table_model.moc"
