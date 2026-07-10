// tst_mirror_window_model — the class-W adapter mirror::WindowModel<Entity> (spec 09 §4.6/§8.1)
// and the demand-paging signal seam. Runs under QAbstractItemModelTester (§12). Covers: cursor-
// ordered initial population from A1's boxed window items, incremental append (insert at tail),
// head eviction reaching the model as a remove (non-journaled per §4.6, observed via the sweep),
// and the olderRequested / canFetchMore / fetchMore seam A4 fulfils.

#include "mirror/observe_coarse.h"
#include "mirror/store.h"
#include "mirror/window_model.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QString>
#include <QTest>

using namespace mirror;

namespace {
ChatMessage chat(const QString& transport, const QString& convId, quint64 cursor,
                 const QString& text) {
    ChatMessage m;
    m.transport = transport;
    m.conv = convId;
    m.cursor = cursor;
    m.text = text;
    return m;
}
const ChatMessageScope kScope{QStringLiteral("m"), QStringLiteral("!room")};
} // namespace

class TstMirrorWindowModel : public QObject {
    Q_OBJECT

private slots:
    void initialPopulationInCursorOrder() {
        CoarseObserve obs;
        Store store(obs);
        auto b = store.beginBatch();
        for (quint64 cur = 1; cur <= 4; ++cur) {
            b.appendWindow(chat(QStringLiteral("m"), QStringLiteral("!room"), cur,
                                QStringLiteral("msg%1").arg(cur)));
        }
        b.commit();

        WindowModel<ChatMessage> model(store, kScope);
        QAbstractItemModelTester tester(&model);
        QCOMPARE(model.rowCount(), 4);
        const int textRole = [&] {
            const auto names = model.roleNames();
            for (auto it = names.constBegin(); it != names.constEnd(); ++it) {
                if (it.value() == "text") {
                    return it.key();
                }
            }
            return -1;
        }();
        QCOMPARE(model.data(model.index(0, 0), textRole).toString(), QStringLiteral("msg1"));
        QCOMPARE(model.data(model.index(3, 0), textRole).toString(), QStringLiteral("msg4"));
    }

    void appendInsertsAtTail() {
        CoarseObserve obs;
        Store store(obs);
        WindowModel<ChatMessage> model(store, kScope);
        QAbstractItemModelTester tester(&model);
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        QSignalSpy resetSpy(&model, &QAbstractItemModel::modelAboutToBeReset);

        store.beginBatch()
            .appendWindow(
                chat(QStringLiteral("m"), QStringLiteral("!room"), 1, QStringLiteral("first")))
            .commit();
        store.beginBatch()
            .appendWindow(
                chat(QStringLiteral("m"), QStringLiteral("!room"), 2, QStringLiteral("second")))
            .commit();

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(inserted.count(), 2);
        QCOMPARE(resetSpy.count(), 0);
    }

    void otherScopeDoesNotDisturbThisModel() {
        CoarseObserve obs;
        Store store(obs);
        WindowModel<ChatMessage> model(store, kScope);
        QAbstractItemModelTester tester(&model);

        store.beginBatch()
            .appendWindow(
                chat(QStringLiteral("m"), QStringLiteral("!other"), 1, QStringLiteral("elsewhere")))
            .commit();
        QCOMPARE(model.rowCount(), 0); // scope filter (§8.1) keeps foreign deltas out
    }

    void headEvictionReachesModelAsRemove() {
        CoarseObserve obs;
        Store store(obs);
        store.setWindowMaxItems(EntityKind::ChatMessage, 3);
        WindowModel<ChatMessage> model(store, kScope);
        QAbstractItemModelTester tester(&model);
        QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);

        for (quint64 cur = 1; cur <= 5; ++cur) {
            store.beginBatch()
                .appendWindow(chat(QStringLiteral("m"), QStringLiteral("!room"), cur,
                                   QStringLiteral("m%1").arg(cur)))
                .commit();
        }
        // Window capped at 3: cursors 1,2 evicted from the head, reaching the model as removes.
        QCOMPARE(model.rowCount(), 3);
        QVERIFY(removed.count() >= 2);
        const int cursorRole = [&] {
            const auto names = model.roleNames();
            for (auto it = names.constBegin(); it != names.constEnd(); ++it) {
                if (it.value() == "cursor") {
                    return it.key();
                }
            }
            return -1;
        }();
        QCOMPARE(model.data(model.index(0, 0), cursorRole).toULongLong(), quint64(3));
    }

    void demandPagingSeam() {
        CoarseObserve obs;
        Store store(obs);
        WindowModel<ChatMessage> model(store, kScope);
        QAbstractItemModelTester tester(&model);
        QSignalSpy older(&model, &WindowModel<ChatMessage>::olderRequested);

        QVERIFY(model.canFetchMore(QModelIndex()));
        model.requestOlder(32);
        QCOMPARE(older.count(), 1);
        QCOMPARE(older.at(0).at(0).toString(), kScope.serialize());
        QCOMPARE(older.at(0).at(1).toInt(), 32);

        model.fetchMore(QModelIndex());
        QCOMPARE(older.count(), 2);
        QCOMPARE(older.at(1).at(1).toInt(), 64); // default page size

        // A4 marks the scope fully back-filled: canFetchMore goes false.
        model.setHasMoreOlder(false);
        QVERIFY(!model.canFetchMore(QModelIndex()));
    }
};

QTEST_MAIN(TstMirrorWindowModel)
#include "tst_mirror_window_model.moc"
