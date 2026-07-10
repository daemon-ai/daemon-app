// tst_mirror_parity — dual-write parity asserts (spec 09 §13 M1 exit criterion). The mirror's
// per-domain row-set is compared to the legacy repository's row-set at quiesce; a mismatch logs
// (dev) and is observable. Deterministic, headless.

#include "mirror/observe_coarse.h"
#include "mirror/parity.h"
#include "mirror/store.h"

#include <QObject>
#include <QTest>

using namespace mirror;

namespace {
Conversation conv(const QString& t, const QString& id) {
    Conversation c;
    c.transport = t;
    c.id = id;
    return c;
}
} // namespace

class TstMirrorParity : public QObject {
    Q_OBJECT

private slots:
    void identicalRowSetsMatch() {
        const QSet<QString> a = {QStringLiteral("m\x1f!a"), QStringLiteral("m\x1f!b")};
        const parity::Result r = parity::compareKeys(a, a);
        QVERIFY(r.matches());
        QVERIFY(parity::checkAndLog(QStringLiteral("conversations"), r));
    }

    void divergenceIsReportedBothDirections() {
        const QSet<QString> mirrorKeys = {QStringLiteral("m\x1f!a"), QStringLiteral("m\x1f!only")};
        const QSet<QString> legacyKeys = {QStringLiteral("m\x1f!a"), QStringLiteral("m\x1f!other")};
        const parity::Result r = parity::compareKeys(mirrorKeys, legacyKeys);
        QVERIFY(!r.matches());
        QCOMPARE(r.onlyInMirror, QStringList{QStringLiteral("m\x1f!only")});
        QCOMPARE(r.onlyInLegacy, QStringList{QStringLiteral("m\x1f!other")});
        QVERIFY(!parity::checkAndLog(QStringLiteral("conversations"), r));
    }

    void conversationKeysExtractedFromSnapshotIndex() {
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a")))
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!b")))
            .upsert(conv(QStringLiteral("slack"), QStringLiteral("#c")))
            .commit();
        const QSet<QString> keys = parity::conversationKeys(store.snapshot(), QStringLiteral("m"));
        QCOMPARE(keys.size(), 2);
        QVERIFY(keys.contains(QStringLiteral("m\x1f!a")));
        QVERIFY(keys.contains(QStringLiteral("m\x1f!b")));

        // A mirror that matches the legacy transport-registry conversation list passes parity.
        const QSet<QString> legacy = {QStringLiteral("m\x1f!a"), QStringLiteral("m\x1f!b")};
        QVERIFY(parity::compareKeys(keys, legacy).matches());
    }
};

QTEST_GUILESS_MAIN(TstMirrorParity)
#include "tst_mirror_parity.moc"
