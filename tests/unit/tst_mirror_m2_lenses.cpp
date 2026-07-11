// tst_mirror_m2_lenses (mirror A5) — the M2 read lenses on the mirror (spec 09 §8.1/§8.3, §4.6).
// ChatWindowModel (cursor-ordered window + demand-paging seam), PersonListModel, ContactListModel
// (transport-filtered) — all curated role maps reading typed entity fields (no QVariantMap shape),
// exact row ops under QAbstractItemModelTester (§12), the single canonical shapes retiring the
// 5→1 / 4→1 layout forks (07§6).

#include "mirror/chat_window_model.h"
#include "mirror/contact_list_model.h"
#include "mirror/observe_coarse.h"
#include "mirror/person_list_model.h"
#include "mirror/store.h"

#include <QAbstractItemModelTester>
#include <QObject>
#include <QSignalSpy>
#include <QTest>

using namespace mirror;

namespace {
ChatMessage chat(const QString& t, const QString& c, quint64 cursor, const QString& author,
                 const QString& text) {
    ChatMessage m;
    m.transport = t;
    m.conv = c;
    m.cursor = cursor;
    m.author = author;
    m.text = text;
    return m;
}
Person person(const QString& id, const QString& alias) {
    Person p;
    p.id = id;
    p.alias = alias;
    return p;
}
Contact contact(const QString& t, const QString& id, const QString& name) {
    Contact c;
    c.transport = t;
    c.id = id;
    c.display_name = name;
    return c;
}
} // namespace

class TstMirrorM2Lenses : public QObject {
    Q_OBJECT

private slots:
    void chatWindowCursorOrderedRoleMap() {
        CoarseObserve obs;
        Store store(obs);
        // Seed two messages before the model attaches (initial population).
        store.beginBatch()
            .appendWindow(chat("m", "!r", 1, "alice", "first"))
            .appendWindow(chat("m", "!r", 2, "bob", "second"))
            .commit();

        ChatWindowModel model(store, ChatMessageScope{QStringLiteral("m"), QStringLiteral("!r")});
        QAbstractItemModelTester tester(&model,
                                        QAbstractItemModelTester::FailureReportingMode::Fatal);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0, 0), ChatWindowModel::TextRole).toString(),
                 QStringLiteral("first"));
        QCOMPARE(model.data(model.index(1, 0), ChatWindowModel::AuthorRole).toString(),
                 QStringLiteral("bob"));

        // A new message appends at the tail as an EXACT insert (no reset) via the journal delta.
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        QSignalSpy reset(&model, &QAbstractItemModel::modelAboutToBeReset);
        store.beginBatch().appendWindow(chat("m", "!r", 3, "carol", "third")).commit();
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(inserted.count(), 1);
        QCOMPARE(reset.count(), 0);
        QCOMPARE(model.data(model.index(2, 0), ChatWindowModel::TextRole).toString(),
                 QStringLiteral("third"));
    }

    void chatWindowDemandPagingSeam() {
        CoarseObserve obs;
        Store store(obs);
        ChatWindowModel model(store, ChatMessageScope{QStringLiteral("m"), QStringLiteral("!r")});
        QSignalSpy older(&model, &TableModelBase::olderRequested);
        // A cold window offers fetchMore (has-more default true); fetchMore emits olderRequested
        // (the ingestor fulfils it, §4.6). requestOlder() is the explicit invokable.
        QVERIFY(model.canFetchMore(QModelIndex()));
        model.fetchMore(QModelIndex());
        QVERIFY(older.count() >= 1);
        QCOMPARE(older.at(0).at(0).toString(), QStringLiteral("m\x1f!r"));
    }

    void personListAliasSorted() {
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch().upsert(person("p2", "Zoe")).upsert(person("p1", "Amy")).commit();
        PersonListModel model(store);
        QAbstractItemModelTester tester(&model,
                                        QAbstractItemModelTester::FailureReportingMode::Fatal);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0, 0), PersonListModel::AliasRole).toString(),
                 QStringLiteral("Amy"));
        QCOMPARE(model.data(model.index(1, 0), PersonListModel::AliasRole).toString(),
                 QStringLiteral("Zoe"));
    }

    void contactListTransportFiltered() {
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(contact("m", "@a", "Alice"))
            .upsert(contact("m", "@b", "Bob"))
            .upsert(contact("x", "@z", "Zoe")) // other transport
            .commit();
        ContactListModel model(store, QStringLiteral("m"));
        QAbstractItemModelTester tester(&model,
                                        QAbstractItemModelTester::FailureReportingMode::Fatal);
        QCOMPARE(model.rowCount(), 2); // only transport 'm'
        QCOMPARE(model.data(model.index(0, 0), ContactListModel::DisplayNameRole).toString(),
                 QStringLiteral("Alice"));

        // A live upsert for 'm' inserts; an upsert for 'x' is filtered out (no row).
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        store.beginBatch().upsert(contact("m", "@c", "Cara")).commit();
        QCOMPARE(model.rowCount(), 3);
        store.beginBatch().upsert(contact("x", "@y", "Yan")).commit();
        QCOMPARE(model.rowCount(), 3); // unchanged — other transport
    }
};

QTEST_GUILESS_MAIN(TstMirrorM2Lenses)
#include "tst_mirror_m2_lenses.moc"
