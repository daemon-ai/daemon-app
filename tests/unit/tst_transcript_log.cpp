// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "app/transcript_log.h"
#include "core/document_store.h"
#include "daemonnet/seed_transcripts.h"
#include "domain/session_log.h"

#include <QtTest>

using be::applyTranscriptLog;
using be::decomposeMarkdown;
using be::DocumentStore;

// Guards the P4 transcript<->SessionLogEntry bridge: rendering a session from its
// decomposed entry sequence reproduces the same document the markdown path does.
// The keystone is a round-trip equality - decompose(md) -> apply -> toMarkdown must
// equal the markdown path's normalized form - so fidelity is provable without a render.
class TestTranscriptLog : public QObject {
    Q_OBJECT

    // The markdown path's canonical form (parse + reserialize).
    static QString normalized(const QString& md) {
        DocumentStore doc;
        doc.loadMarkdown(md);
        return doc.toMarkdown();
    }
    // The entry path: decompose to a SessionLogEntry sequence, then rebuild the document.
    static QString viaEntries(const QString& md) {
        DocumentStore doc;
        applyTranscriptLog(doc, decomposeMarkdown(md));
        return doc.toMarkdown();
    }

private slots:
    // The agent-block showcase (reasoning, every tool detail kind, clarify, approval,
    // image, content stream) - all un-roled - reproduces exactly via entries.
    void agentBlocksRoundTrips() {
        const QString md = daemonnet::seed::agentBlocksMarkdown();
        QCOMPARE(viaEntries(md), normalized(md));
    }

    // The message/role layer (user/assistant/system turns, steer/slash/process) - the
    // part the streaming ingest can't do - reproduces exactly via entries.
    void roleLayerRoundTrips() {
        const QString md = daemonnet::seed::roleLayerMarkdown();
        QCOMPARE(viaEntries(md), normalized(md));
    }

    // A short plain-prose session round-trips (the common case).
    void plainProseRoundTrips() {
        const QString md = QStringLiteral("# Title\n\nA paragraph.\n\n- one\n- two\n");
        QCOMPARE(viaEntries(md), normalized(md));
    }

    // Decompose yields one entry per block, carrying the role envelope + payload.
    void decomposeCarriesRoleAndPayload() {
        const auto entries = decomposeMarkdown(daemonnet::seed::roleLayerMarkdown());
        QVERIFY(!entries.isEmpty());
        bool sawUser = false;
        bool sawAssistant = false;
        bool sawSystem = false;
        for (const domain::SessionLogEntry& e : entries) {
            const QString role = e.payload.value(QStringLiteral("role")).toString();
            sawUser = sawUser || role == QStringLiteral("user");
            sawAssistant = sawAssistant || role == QStringLiteral("assistant");
            sawSystem = sawSystem || role == QStringLiteral("system");
            QVERIFY(e.payload.contains(QStringLiteral("markdown")));
        }
        QVERIFY(sawUser);
        QVERIFY(sawAssistant);
        QVERIFY(sawSystem);
        // A user turn is inbound; assistant/system are outbound.
        QVERIFY(std::any_of(entries.begin(), entries.end(), [](const domain::SessionLogEntry& e) {
            return e.direction == domain::Direction::Inbound;
        }));
    }
};

QTEST_MAIN(TestTranscriptLog)
#include "tst_transcript_log.moc"
