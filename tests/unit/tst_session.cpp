// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "session/mock_checkpoint_timeline.h"
#include "session/mock_session_settings.h"
#include "uimodels/variant_list_model.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

using session::MockCheckpointTimeline;
using session::MockSessionSettings;
using uimodels::VariantListModel;

// Guards the Phase 8 composer seams: per-session overrides (with dedup'd change
// signal) and the checkpoint timeline (restore drops later entries + sets current).
class TestSession : public QObject {
    Q_OBJECT

    static VariantListModel* asModel(QObject* o) { return qobject_cast<VariantListModel*>(o); }

private slots:
    void sessionSettingsRoundTrip() {
        MockSessionSettings s;
        QSignalSpy spy(&s, &session::ISessionSettings::changed);
        s.setEffort(QStringLiteral("high"));
        s.setFast(true);
        s.setProfile(QStringLiteral("Coder"));
        QCOMPARE(s.effort(), QStringLiteral("high"));
        QVERIFY(s.fast());
        QCOMPARE(s.profile(), QStringLiteral("Coder"));
        QCOMPARE(spy.count(), 3);
        s.setFast(true); // no-op
        QCOMPARE(spy.count(), 3);
        QVERIFY(!s.effortOptions().isEmpty());
    }

    void checkpointRestoreTruncates() {
        MockCheckpointTimeline t;
        const int before = t.count();
        QVERIFY(before >= 3);

        // Restore to the second checkpoint: everything after it is dropped.
        t.restore(QStringLiteral("cp-2"));
        QCOMPARE(t.count(), 2);
        QVERIFY(asModel(t.checkpoints())
                    ->at(asModel(t.checkpoints())->indexOfId(QStringLiteral("cp-2")))
                    .value(QStringLiteral("current"))
                    .toBool());
    }

    void createCheckpointBecomesCurrent() {
        MockCheckpointTimeline t;
        const int before = t.count();
        const QString id = t.createCheckpoint(QStringLiteral("Test"));
        QCOMPARE(t.count(), before + 1);
        QVERIFY(asModel(t.checkpoints())
                    ->at(asModel(t.checkpoints())->indexOfId(id))
                    .value(QStringLiteral("current"))
                    .toBool());
    }

    // Per-session overrides are keyed by session: editing one chat's effort
    // does not leak into another, and switching back restores it.
    void sessionSettingsArePerSession() {
        MockSessionSettings s;
        s.setSessionId(QStringLiteral("s-1"));
        s.setEffort(QStringLiteral("high"));
        s.setProfile(QStringLiteral("Coder"));

        s.setSessionId(QStringLiteral("s-2"));
        // A fresh session starts at the defaults, not chat 1's overrides. The default profile is
        // empty (= the node's active profile drives the turn until the user picks one).
        QCOMPARE(s.effort(), QStringLiteral("medium"));
        QVERIFY(s.profile().isEmpty());
        s.setEffort(QStringLiteral("low"));

        // Switching back restores chat 1's overrides.
        s.setSessionId(QStringLiteral("s-1"));
        QCOMPARE(s.effort(), QStringLiteral("high"));
        QCOMPARE(s.profile(), QStringLiteral("Coder"));
    }

    // The checkpoint timeline is per-session: a rewind in one chat does not
    // shorten another's, and switching back shows the rewound timeline.
    void checkpointTimelineIsPerSession() {
        MockCheckpointTimeline t;
        t.setSessionId(QStringLiteral("s-1"));
        const int full = t.count();
        QVERIFY(full >= 3);
        t.restore(QStringLiteral("cp-2"));
        QCOMPARE(t.count(), 2);

        // Chat 2 has its own full timeline, untouched by chat 1's rewind.
        t.setSessionId(QStringLiteral("s-2"));
        QCOMPARE(t.count(), full);

        // Back to chat 1: still rewound to two entries.
        t.setSessionId(QStringLiteral("s-1"));
        QCOMPARE(t.count(), 2);
    }
};

QTEST_MAIN(TestSession)
#include "tst_session.moc"
