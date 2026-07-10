// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// A2 (mirror architecture): the composer turn-prompt lane storage swap (spec 09 §6.7 step 1).
// Proves that binding ComposerQueueModel's storage to the outbox turn-prompt lane makes queued
// prompts crash-durable WITHOUT changing the model's roles/signals/behavior — consumers cannot
// tell. The existing composer suites (tst_composer_session, tst_completion) are the untouched
// regression harness for the default in-memory path; this test covers the durable path.

#include "composer_queue_model.h"
#include "local_database.h"
#include "outbox.h"
#include "outbox_composer_queue_storage.h"
#include "verb_class.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace mirror;

class TestComposerOutboxQueue : public QObject {
    Q_OBJECT

private slots:
    void init() {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
    }
    void cleanup() { m_dir.reset(); }
    QString dbPath() const { return m_dir->filePath(QStringLiteral("local.db")); }

    // Default (no storage) path is unchanged — a plain in-memory queue.
    void inMemoryUnchangedByDefault() {
        ComposerQueueModel q;
        QSignalSpy count(&q, &ComposerQueueModel::countChanged);
        q.append(QStringLiteral("one"), QString());
        q.append(QStringLiteral("two"), QStringLiteral("@file:x"));
        QCOMPARE(q.count(), 2);
        QCOMPARE(q.textAt(0), QStringLiteral("one"));
        QCOMPARE(q.refsAt(1), QStringLiteral("@file:x"));
        QVERIFY(count.count() >= 2);
    }

    // Payload encode/decode round-trips (text + refs).
    void entryEncodeRoundTrip() {
        ComposerQueueModel::Entry e{QStringLiteral("hello\nworld"), QStringLiteral("@image:y")};
        const QByteArray bytes = OutboxComposerQueueStorage::encodeEntry(e);
        const ComposerQueueModel::Entry back = OutboxComposerQueueStorage::decodeEntry(bytes);
        QCOMPARE(back.text, e.text);
        QCOMPARE(back.refs, e.refs);
    }

    // With the outbox storage attached, the SAME roles/signals fire, and the entries persist onto
    // the per-session turn-prompt lane.
    void durableStoragePreservesRolesAndSignals() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.load();
        OutboxComposerQueueStorage storage(&ob, QStringLiteral("sess-1"));

        ComposerQueueModel q;
        q.setStorage(&storage);
        QSignalSpy count(&q, &ComposerQueueModel::countChanged);
        QSignalSpy inserted(&q, &QAbstractListModel::rowsInserted);

        q.append(QStringLiteral("first"), QString());
        q.append(QStringLiteral("second"), QStringLiteral("@file:a"));
        QCOMPARE(q.count(), 2);
        QVERIFY(inserted.count() >= 2);
        QVERIFY(count.count() >= 2);
        // Roles are the ORIGINAL ones (consumers untouched).
        const auto roles = q.roleNames();
        QCOMPARE(roles.value(ComposerQueueModel::TextRole), QByteArrayLiteral("text"));
        QCOMPARE(roles.value(ComposerQueueModel::RefsRole), QByteArrayLiteral("refs"));

        // The entries landed on the turn-prompt lane (durable).
        const QString lane = buildLane(VerbClass::TurnPrompt, QStringLiteral("sess-1"));
        QCOMPARE(ob.laneDepth(lane), 2);
    }

    // Crash-durability: after a simulated restart the queued prompts reappear in FIFO order.
    void durableQueueSurvivesRestart() {
        const QString path = dbPath();
        {
            LocalDatabase db(path);
            Outbox ob(&db);
            ob.load();
            OutboxComposerQueueStorage storage(&ob, QStringLiteral("sess-1"));
            ComposerQueueModel q;
            q.setStorage(&storage);
            q.append(QStringLiteral("alpha"), QString());
            q.append(QStringLiteral("beta"), QStringLiteral("@file:b"));
            // CRASH.
        }
        LocalDatabase db2(path);
        Outbox ob2(&db2);
        ob2.load();
        OutboxComposerQueueStorage storage2(&ob2, QStringLiteral("sess-1"));
        ComposerQueueModel q2;
        q2.setStorage(&storage2); // hydrates from the durable lane
        QCOMPARE(q2.count(), 2);
        QCOMPARE(q2.textAt(0), QStringLiteral("alpha"));
        QCOMPARE(q2.textAt(1), QStringLiteral("beta"));
        QCOMPARE(q2.refsAt(1), QStringLiteral("@file:b"));
    }

    // Removing (as the drain does) updates the durable lane too.
    void removeUpdatesDurableLane() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.load();
        OutboxComposerQueueStorage storage(&ob, QStringLiteral("sess-1"));
        ComposerQueueModel q;
        q.setStorage(&storage);
        q.append(QStringLiteral("a"), QString());
        q.append(QStringLiteral("b"), QString());
        q.removeAt(0);
        QCOMPARE(q.count(), 1);
        const QString lane = buildLane(VerbClass::TurnPrompt, QStringLiteral("sess-1"));
        QCOMPARE(ob.laneDepth(lane), 1);
    }

private:
    std::unique_ptr<QTemporaryDir> m_dir;
};

QTEST_MAIN(TestComposerOutboxQueue)
#include "tst_composer_outbox_queue.moc"
