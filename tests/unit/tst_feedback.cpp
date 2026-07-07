// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// The user-feedback seam's in-memory stand-in (MockFeedback): it records every
// message/app submission for introspection, owns the telemetry-consent source of
// truth (persisted so it survives a restart), and only flips telemetry on through
// the app-feedback dialog's explicit opt-in - never as a silent side effect.

#include "appcache/json_cache.h"
#include "feedback/mock_feedback.h"

#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QtTest>

class TestFeedback : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Isolate the consent cache from the real user data.
        QStandardPaths::setTestModeEnabled(true);
    }

    void init() {
        // Each test starts from a pristine (telemetry-off) consent cache.
        QFile::remove(appcache::path(QStringLiteral("feedback_consent.json")));
    }

    void recordsMessageFeedback() {
        feedback::MockFeedback fb;
        QSignalSpy submitted(&fb, &feedback::IFeedback::submitted);

        QVariantMap anchor;
        anchor.insert(QStringLiteral("turnSeq"), 7);
        anchor.insert(QStringLiteral("journalCursor"), QStringLiteral("jc-3"));
        fb.submitMessageFeedback(QStringLiteral("s-1"), anchor, feedback::IFeedback::kRatingUp,
                                 QStringLiteral("nice"), /*includeContent=*/false);

        QCOMPARE(fb.messageRecords().size(), 1);
        const feedback::MockFeedback::MessageRecord& rec = fb.messageRecords().first();
        QCOMPARE(rec.sessionId, QStringLiteral("s-1"));
        QCOMPARE(rec.rating, feedback::IFeedback::kRatingUp);
        QCOMPARE(rec.comment, QStringLiteral("nice"));
        QVERIFY(!rec.includeContent);
        QCOMPARE(rec.anchor.value(QStringLiteral("turnSeq")).toInt(), 7);
        QCOMPARE(rec.anchor.value(QStringLiteral("journalCursor")).toString(),
                 QStringLiteral("jc-3"));

        QCOMPARE(submitted.size(), 1);
        QCOMPARE(submitted.first().first().toString(), QStringLiteral("message"));

        // A follow-up submission (e.g. the same message re-rated with a comment and the
        // response-content opt-in) is recorded as its own entry; explicit feedback flows
        // even with telemetry off.
        QVERIFY(!fb.telemetryEnabled());
        fb.submitMessageFeedback(QStringLiteral("s-1"), anchor, feedback::IFeedback::kRatingDown,
                                 QStringLiteral("changed my mind"), /*includeContent=*/true);
        QCOMPARE(fb.messageRecords().size(), 2);
        QCOMPARE(fb.messageRecords().last().rating, feedback::IFeedback::kRatingDown);
        QVERIFY(fb.messageRecords().last().includeContent);
    }

    void recordsAppFeedback() {
        feedback::MockFeedback fb;
        QSignalSpy submitted(&fb, &feedback::IFeedback::submitted);

        fb.submitAppFeedback(QStringLiteral("bug"), QStringLiteral("it crashed"),
                             /*includeDiagnostics=*/true, /*alsoEnableTelemetry=*/false);

        QCOMPARE(fb.appRecords().size(), 1);
        const feedback::MockFeedback::AppRecord& rec = fb.appRecords().first();
        QCOMPARE(rec.category, QStringLiteral("bug"));
        QCOMPARE(rec.comment, QStringLiteral("it crashed"));
        QVERIFY(rec.includeDiagnostics);
        QVERIFY(!rec.alsoEnableTelemetry);
        QCOMPARE(submitted.size(), 1);
        QCOMPARE(submitted.first().first().toString(), QStringLiteral("app"));

        // A plain app submission with the opt-in unchecked never turns telemetry on.
        QVERIFY(!fb.telemetryEnabled());
    }

    void consentFlipEmitsDedupsAndPersists() {
        {
            feedback::MockFeedback fb;
            QVERIFY(!fb.telemetryEnabled()); // default off on a first run
            QSignalSpy changed(&fb, &feedback::IFeedback::telemetryEnabledChanged);

            fb.setTelemetryEnabled(true);
            QVERIFY(fb.telemetryEnabled());
            QCOMPARE(changed.size(), 1);
            QCOMPARE(changed.first().first().toBool(), true);

            // Setting the same value again is a no-op (no duplicate signal).
            fb.setTelemetryEnabled(true);
            QCOMPARE(changed.size(), 1);
        }

        // A fresh instance restores the persisted consent (survives a restart).
        feedback::MockFeedback restored;
        QVERIFY(restored.telemetryEnabled());
    }

    void appFeedbackOptInEnablesTelemetry() {
        feedback::MockFeedback fb;
        QVERIFY(!fb.telemetryEnabled());
        QSignalSpy changed(&fb, &feedback::IFeedback::telemetryEnabledChanged);

        // The dialog's explicit, default-unchecked opt-in is the ONLY feedback path
        // that may enable telemetry.
        fb.submitAppFeedback(QStringLiteral("idea"), QStringLiteral("please add X"),
                             /*includeDiagnostics=*/false, /*alsoEnableTelemetry=*/true);
        QVERIFY(fb.telemetryEnabled());
        QCOMPARE(changed.size(), 1);
        QCOMPARE(fb.appRecords().size(), 1);
        QVERIFY(fb.appRecords().first().alsoEnableTelemetry);

        // Already-on + opt-in checked again does not re-emit (idempotent).
        fb.submitAppFeedback(QStringLiteral("other"), QStringLiteral("more"),
                             /*includeDiagnostics=*/false, /*alsoEnableTelemetry=*/true);
        QCOMPARE(changed.size(), 1);
    }
};

QTEST_GUILESS_MAIN(TestFeedback)
#include "tst_feedback.moc"
