// tst_mirror_stable_id — the 64-bit stable-id interner (spec 09 §8.1). Proves the property that
// fixes Sink's 32-bit qHash(resourceId+entityId) collision defect
// (references/architecture/sink/common/modelresult.cpp:32-41, anti-model): ids are monotonic,
// never reused, and bijective for the interner's lifetime (behavior re-expressed, no borrowed
// text — Sink is LGPL).

#include "mirror/stable_id.h"

#include <QSet>
#include <QString>
#include <QTest>

using namespace mirror;

class TstMirrorStableId : public QObject {
    Q_OBJECT

private slots:
    void assignsMonotonicallyFromOne() {
        StableIdInterner in;
        QCOMPARE(in.intern(QStringLiteral("a")), qint64(1));
        QCOMPARE(in.intern(QStringLiteral("b")), qint64(2));
        QCOMPARE(in.intern(QStringLiteral("c")), qint64(3));
        QCOMPARE(in.size(), 3);
    }

    void internIsIdempotentPerKey() {
        StableIdInterner in;
        const qint64 a1 = in.intern(QStringLiteral("k"));
        const qint64 a2 = in.intern(QStringLiteral("k"));
        QCOMPARE(a1, a2);
        QCOMPARE(in.size(), 1);
    }

    void distinctKeysNeverCollide() {
        // The whole point vs Sink: no two distinct keys ever map to the same id.
        StableIdInterner in;
        QSet<qint64> ids;
        for (int i = 0; i < 5000; ++i) {
            ids.insert(in.intern(QStringLiteral("key-%1").arg(i)));
        }
        QCOMPARE(ids.size(), 5000);
    }

    void lookupDoesNotAssign() {
        StableIdInterner in;
        QCOMPARE(in.lookup(QStringLiteral("ghost")), StableIdInterner::kInvalidId);
        QVERIFY(!in.contains(QStringLiteral("ghost")));
        QCOMPARE(in.size(), 0);
        in.intern(QStringLiteral("real"));
        QCOMPARE(in.lookup(QStringLiteral("real")), qint64(1));
    }

    void idsAreNeverReused() {
        // After forgetting a key, re-interning it yields a NEW higher id — never the old one.
        StableIdInterner in;
        const qint64 first = in.intern(QStringLiteral("x")); // 1
        (void)in.intern(QStringLiteral("y"));                // 2
        in.forget(QStringLiteral("x"));
        const qint64 reassigned = in.intern(QStringLiteral("x")); // 3, not 1
        QVERIFY(reassigned != first);
        QCOMPARE(reassigned, qint64(3));
    }
};

QTEST_MAIN(TstMirrorStableId)
#include "tst_mirror_stable_id.moc"
