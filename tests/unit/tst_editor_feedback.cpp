// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// EditorController's per-message thumbs-feedback API (the "feedback over
// OpenTelemetry" feature). The controller records the rating locally (so a
// recycled footer re-renders its selected glyph via ratingFor), resolves the
// message's wire anchor from its TranscriptIngest, and emits the host signals
// Transcript.qml forwards to the Feedback seam - without any IFeedback
// dependency of its own. The anchor CAPTURE itself is covered by the ingest test
// (be_core_tests ingestCapturesAnchorMetadata); here we prove the controller
// forwards the resolved anchor for a live-ingested message.

#include "app/editor_controller.h"

#include <QSignalSpy>
#include <QtTest>
#include <QVariantList>
#include <QVariantMap>

using be::app::EditorController;

class EditorFeedbackTests : public QObject {
    Q_OBJECT

private slots:
    // A submission records the rating (ratingFor reflects it), emits
    // messageRatingChanged + messageFeedbackSubmitted, and a re-rate updates the
    // stored value in place (last write wins - the footer toggles up<->down).
    void ratingRoundTripAndUpdate() {
        EditorController ed;
        QCOMPARE(ed.ratingFor(QStringLiteral("m1")), 0); // none until submitted

        QSignalSpy ratingChanged(&ed, &EditorController::messageRatingChanged);
        QSignalSpy submitted(&ed, &EditorController::messageFeedbackSubmitted);

        ed.submitMessageFeedback(QStringLiteral("m1"), 1, QString());
        QCOMPARE(ed.ratingFor(QStringLiteral("m1")), 1);
        QCOMPARE(ratingChanged.size(), 1);
        QCOMPARE(ratingChanged.first().at(0).toString(), QStringLiteral("m1"));
        QCOMPARE(ratingChanged.first().at(1).toInt(), 1);
        QCOMPARE(submitted.size(), 1);
        QCOMPARE(submitted.first().at(0).toString(), QStringLiteral("m1"));
        QCOMPARE(submitted.first().at(2).toInt(), 1);
        QCOMPARE(submitted.first().at(3).toString(), QString());

        // Re-rate the same message with a comment: the stored rating updates and
        // the submission carries the comment.
        ed.submitMessageFeedback(QStringLiteral("m1"), -1, QStringLiteral("actually bad"));
        QCOMPARE(ed.ratingFor(QStringLiteral("m1")), -1);
        QCOMPARE(ratingChanged.size(), 2);
        QCOMPARE(submitted.size(), 2);
        QCOMPARE(submitted.last().at(2).toInt(), -1);
        QCOMPARE(submitted.last().at(3).toString(), QStringLiteral("actually bad"));

        // A different message keeps its own independent rating.
        QCOMPARE(ed.ratingFor(QStringLiteral("m2")), 0);
    }

    // An empty message id is a no-op: no rating stored, no signals emitted.
    void emptyMessageIdIsNoop() {
        EditorController ed;
        QSignalSpy submitted(&ed, &EditorController::messageFeedbackSubmitted);
        ed.submitMessageFeedback(QString(), 1, QStringLiteral("x"));
        QCOMPARE(submitted.size(), 0);
        QCOMPARE(ed.ratingFor(QString()), 0);
    }

    // The controller resolves the wire anchor a live-ingested turn captured: the
    // first assistant turn of a fresh controller opens message "m1", so feedback
    // for it carries the seq/trace anchor from the turn-opening event.
    void anchorResolvedFromIngest() {
        EditorController ed;

        QVariantMap open;
        open.insert(QStringLiteral("type"), QStringLiteral("text"));
        open.insert(QStringLiteral("text"), QStringLiteral("Working on it.\n"));
        open.insert(QStringLiteral("seq"), 99);
        open.insert(QStringLiteral("traceId"), QStringLiteral("tr-1"));
        QVariantMap flush;
        flush.insert(QStringLiteral("type"), QStringLiteral("flush"));
        ed.ingestEvents(QVariantList{open, flush});

        QSignalSpy submitted(&ed, &EditorController::messageFeedbackSubmitted);
        ed.submitMessageFeedback(QStringLiteral("m1"), 1, QString());
        QCOMPARE(submitted.size(), 1);
        const QVariantMap anchor = submitted.first().at(1).toMap();
        QCOMPARE(anchor.value(QStringLiteral("turnSeq")).toInt(), 99);
        QCOMPARE(anchor.value(QStringLiteral("traceId")).toString(), QStringLiteral("tr-1"));
    }

    // A message with no captured anchor (unknown id / reloaded transcript) resolves
    // to an empty anchor - the documented contract.
    void unknownMessageAnchorIsEmpty() {
        EditorController ed;
        QSignalSpy submitted(&ed, &EditorController::messageFeedbackSubmitted);
        ed.submitMessageFeedback(QStringLiteral("no-such-message"), 1, QString());
        QCOMPARE(submitted.size(), 1);
        QVERIFY(submitted.first().at(1).toMap().isEmpty());
    }
};

QTEST_MAIN(EditorFeedbackTests)
#include "tst_editor_feedback.moc"
