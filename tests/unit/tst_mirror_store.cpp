// tst_mirror_store — the store apply/commit pipeline + the ACTIVE journal integration
// (spec 09 §4.1/§4.2/§4.3, §5.3 diff-before-write, §4.6 windows) and the §4.4 property test
// (journal replay over the previous snapshot == new snapshot).
//
// The immer::table upsert / partial-patch / transient-bulk / diff behaviors are re-expressed
// (adapted, BSL-1.0, attribution) from immer's own suite:
//   - upsert + partial patch: references/architecture/immer/test/table/generic.ipp:134-147,
//     268-302,496-571
//   - transient bulk insert:  references/architecture/immer/test/table_transient/generic.ipp:35-93
//   - diff added/removed/changed: references/architecture/immer/test/algorithm.cpp:72-98 +
//     test/table/generic.ipp:549-571
// immer is BSL-1.0 (no attribution requirement); credited here for provenance regardless.

#include "mirror/observe_coarse.h"
#include "mirror/store.h"

#include <immer/algorithm.hpp>
#include <QObject>
#include <QSet>
#include <QSignalSpy>
#include <QString>
#include <QTest>
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

ChatMessage chat(const QString& transport, const QString& convId, quint64 cursor,
                 const QString& text) {
    ChatMessage m;
    m.transport = transport;
    m.conv = convId;
    m.cursor = cursor;
    m.text = text;
    return m;
}
} // namespace

class TstMirrorStore : public QObject {
    Q_OBJECT

private slots:
    void upsertStampsAndPublishesOnce() {
        CoarseObserve obs;
        Store store(obs);
        QSignalSpy spy(&store, &Store::committed);

        auto batch = store.beginBatch();
        batch.upsert(conv(QStringLiteral("matrix-1"), QStringLiteral("!a"), QStringLiteral("A")));
        batch.upsert(conv(QStringLiteral("matrix-1"), QStringLiteral("!b"), QStringLiteral("B")));
        const quint64 head = batch.commit();

        QCOMPARE(head, quint64(2));
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(2));
        QCOMPARE(spy.count(), 1); // ONE notification wave per batch (§4.2)
        QCOMPARE(obs.commitCount(), 1);
        QCOMPARE(obs.lastCommit().revFrom, quint64(0));
        QCOMPARE(obs.lastCommit().revTo, quint64(2));
        QVERIFY(obs.lastCommit().kinds.contains(EntityKind::Conversation));
        QCOMPARE(store.journal().size(), std::size_t(2));
    }

    void insertIsUpsertReplacingByKey() {
        // immer::table insert = upsert (generic.ipp:388-397 doc / :268-302 behavior).
        CoarseObserve obs;
        Store store(obs);
        {
            auto b = store.beginBatch();
            b.upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("first")));
            b.commit();
        }
        {
            auto b = store.beginBatch();
            b.upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("second")));
            b.commit();
        }
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(1));
        const Conversation* c = store.snapshot().conversations.find(
            ConversationKey{QStringLiteral("m"), QStringLiteral("!a")});
        QVERIFY(c != nullptr);
        QCOMPARE(c->title, QStringLiteral("second"));
        QCOMPARE(store.journal().headRev(), quint64(2)); // one record per real change
    }

    void diffBeforeWriteSuppressesUnchangedUpsert() {
        // §5.3: re-upserting an equal value stamps nothing (immer value equality short-circuits).
        CoarseObserve obs;
        Store store(obs);
        const Conversation c = conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("x"));
        store.beginBatch().upsert(c).commit();
        const quint64 headAfterFirst = store.journal().headRev();

        auto b = store.beginBatch();
        b.upsert(c); // identical
        const quint64 head = b.commit();

        QCOMPARE(b.stagedCount(), 0);
        QCOMPARE(head, headAfterFirst); // no new rev
        QCOMPARE(obs.commitCount(), 1); // second (empty) batch did not publish
    }

    void tombstoneRemovesAndStamps() {
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("x")))
            .commit();

        auto b = store.beginBatch();
        b.tombstone<Conversation>(ConversationKey{QStringLiteral("m"), QStringLiteral("!a")});
        b.commit();

        QCOMPARE(store.snapshot().conversations.size(), std::size_t(0));
        QCOMPARE(store.journal().records().back().op, JournalOp::Tombstone);
    }

    void tombstoneAbsentKeyIsNoop() {
        CoarseObserve obs;
        Store store(obs);
        auto b = store.beginBatch();
        b.tombstone<Conversation>(ConversationKey{QStringLiteral("m"), QStringLiteral("!ghost")});
        QCOMPARE(b.stagedCount(), 0);
    }

    void pageIngestIsOneCommitViaTransient() {
        // §4.2 page ingest = transient bulk insert + ONE commit
        // (table_transient/generic.ipp:35-93 bulk-insert behavior).
        CoarseObserve obs;
        Store store(obs);
        std::vector<Conversation> page;
        for (int i = 0; i < 64; ++i) {
            page.push_back(conv(QStringLiteral("m"), QStringLiteral("!%1").arg(i),
                                QStringLiteral("t%1").arg(i)));
        }
        QSignalSpy spy(&store, &Store::committed);
        auto b = store.beginBatch();
        b.upsertPage(page);
        b.commit();

        QCOMPARE(store.snapshot().conversations.size(), std::size_t(64));
        QCOMPARE(spy.count(), 1); // one commit for the whole page
        QCOMPARE(store.journal().size(), std::size_t(64));
    }

    void conversationIndexMaintainedByApply() {
        // §3.5 worked relation index, maintained ONLY inside apply.
        CoarseObserve obs;
        Store store(obs);
        {
            auto b = store.beginBatch();
            b.upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A")));
            b.upsert(conv(QStringLiteral("m"), QStringLiteral("!b"), QStringLiteral("B")));
            b.upsert(conv(QStringLiteral("slack-1"), QStringLiteral("#c"), QStringLiteral("C")));
            b.commit();
        }
        QVERIFY(store.snapshot().conversationsByTransport.count(QStringLiteral("m")));
        QCOMPARE(store.snapshot().conversationsByTransport[QStringLiteral("m")].size(),
                 std::size_t(2));
        QCOMPARE(store.snapshot().conversationsByTransport[QStringLiteral("slack-1")].size(),
                 std::size_t(1));

        store.beginBatch()
            .tombstone<Conversation>(ConversationKey{QStringLiteral("m"), QStringLiteral("!a")})
            .commit();
        QCOMPARE(store.snapshot().conversationsByTransport[QStringLiteral("m")].size(),
                 std::size_t(1));
    }

    void windowAppendUpdatesMeta() {
        // §4.6 windowed collection: cursor-ordered items + window_meta accounting.
        CoarseObserve obs;
        Store store(obs);
        auto b = store.beginBatch();
        for (quint64 cur = 1; cur <= 5; ++cur) {
            b.appendWindow(chat(QStringLiteral("m"), QStringLiteral("!room"), cur,
                                QStringLiteral("msg%1").arg(cur)));
        }
        b.commit();

        const ChatMessageScope scope{QStringLiteral("m"), QStringLiteral("!room")};
        QVERIFY(store.snapshot().chat.count(scope));
        const auto& win = store.snapshot().chat[scope];
        QCOMPARE(win.items.size(), std::size_t(5));
        QCOMPARE(win.meta.item_count, 5);
        QCOMPARE(win.meta.newest_cursor, quint64(5));
        QCOMPARE(win.meta.oldest_cursor, quint64(1));
        QVERIFY(win.meta.byte_count > 0);
        QCOMPARE(win.items[0].get().text, QStringLiteral("msg1"));
    }

    void windowEvictsOldestPastPolicy() {
        // §4.6 eviction: oldest-first trim past the per-kind item budget (node stays archive).
        CoarseObserve obs;
        Store store(obs);
        store.setWindowMaxItems(EntityKind::ChatMessage, 3);
        auto b = store.beginBatch();
        for (quint64 cur = 1; cur <= 6; ++cur) {
            b.appendWindow(chat(QStringLiteral("m"), QStringLiteral("!room"), cur,
                                QStringLiteral("m%1").arg(cur)));
        }
        b.commit();

        const ChatMessageScope scope{QStringLiteral("m"), QStringLiteral("!room")};
        const auto& win = store.snapshot().chat[scope];
        QCOMPARE(win.items.size(), std::size_t(3));
        QCOMPARE(win.items[0].get().cursor, quint64(4)); // 1..3 evicted
        QCOMPARE(win.meta.newest_cursor, quint64(6));
        QCOMPARE(win.meta.oldest_cursor, quint64(4));
    }

    void immerDiffDetectsAddedRemovedChanged() {
        // immer::diff on tables (algorithm.cpp:72-98 + table/generic.ipp:549-571). Proves the
        // O(changes) change-detection engine the write-behind + parity assert lean on (§8.2).
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A")))
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!b"), QStringLiteral("B")))
            .commit();
        const auto prev = store.snapshot().conversations;

        store.beginBatch()
            .upsert(
                conv(QStringLiteral("m"), QStringLiteral("!b"), QStringLiteral("B2")))    // changed
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!c"), QStringLiteral("C"))) // added
            .tombstone<Conversation>(ConversationKey{QStringLiteral("m"), QStringLiteral("!a")})
            .commit();
        const auto next = store.snapshot().conversations;

        QSet<QString> added, removed, changed;
        immer::diff(prev, next,
                    immer::make_differ(
                        [&](const Conversation& c) { added.insert(c.id); },
                        [&](const Conversation& c) { removed.insert(c.id); },
                        [&](const Conversation&, const Conversation& c) { changed.insert(c.id); }));
        QCOMPARE(added, QSet<QString>{QStringLiteral("!c")});
        QCOMPARE(removed, QSet<QString>{QStringLiteral("!a")});
        QCOMPARE(changed, QSet<QString>{QStringLiteral("!b")});
    }

    void journalReplayEqualsSnapshotDiff() {
        // §4.4 property: replaying the delta set over the previous snapshot yields the new
        // snapshot, AND the journal's changed-key set equals immer::diff's changed-key set.
        CoarseObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A")))
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!b"), QStringLiteral("B")))
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!d"), QStringLiteral("D")))
            .commit();

        const auto prev = store.snapshot().conversations;
        const quint64 revFrom = store.journal().headRev();

        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!b"), QStringLiteral("B*"))) // change
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!c"), QStringLiteral("C")))  // add
            .tombstone<Conversation>(ConversationKey{QStringLiteral("m"), QStringLiteral("!d")})
            .commit();
        const auto next = store.snapshot().conversations;

        // Replay the journal tail over prev, pulling upsert values from the NEW snapshot.
        auto rebuilt = prev;
        QSet<QString> journalKeys;
        for (const JournalRecord& r : store.journal().since(revFrom)) {
            if (r.kind != EntityKind::Conversation) {
                continue;
            }
            journalKeys.insert(r.key);
            // key = transport \x1f id
            const QStringList parts = r.key.split(QChar(0x1f));
            const ConversationKey key{parts.value(0), parts.value(1)};
            if (r.op == JournalOp::Tombstone) {
                rebuilt = rebuilt.erase(key);
            } else {
                const Conversation* v = next.find(key);
                QVERIFY(v != nullptr);
                rebuilt = rebuilt.insert(*v);
            }
        }
        QVERIFY(rebuilt == next); // replay == snapshot

        // journal keys == diff keys
        QSet<QString> diffKeys;
        immer::diff(
            prev, next,
            immer::make_differ([&](const Conversation& c) { diffKeys.insert(c.key().serialize()); },
                               [&](const Conversation& c) { diffKeys.insert(c.key().serialize()); },
                               [&](const Conversation&, const Conversation& c) {
                                   diffKeys.insert(c.key().serialize());
                               }));
        QCOMPARE(journalKeys, diffKeys);
    }

    void deterministicClockStampsAtMs() {
        CoarseObserve obs;
        Store store(obs);
        store.setClock([] { return qint64(123456); });
        store.beginBatch()
            .upsert(conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A")))
            .commit();
        QCOMPARE(store.journal().records().back().at_ms, qint64(123456));
    }
};

QTEST_MAIN(TstMirrorStore)
#include "tst_mirror_store.moc"
