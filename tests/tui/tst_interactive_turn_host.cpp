// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Render honesty (#2): answering an inline approval in the TUI host clears the
// gate via the shared toolApprovalPatch contract and NOTHING ELSE. The tool's
// result (ok/stdout) is never fabricated by the host — in daemon mode it comes
// from the node's real ToolFinished stream event, and in mock mode from the
// TurnController's own scripted toolFinished. These guard against re-introducing
// a locally synthesized success/output after an approval.

#include "core/block_record.h"
#include "core/document_store.h"
#include "interactive_turn_host.h"

#include <QSignalSpy>
#include <QtTest>

class TestInteractiveTurnHost : public QObject {
    Q_OBJECT

private:
    static QVariantMap gatedToolMeta() {
        QVariantMap meta;
        meta.insert(QStringLiteral("callId"), QStringLiteral("c1"));
        meta.insert(QStringLiteral("name"), QStringLiteral("terminal"));
        meta.insert(QStringLiteral("status"), QStringLiteral("running"));
        meta.insert(QStringLiteral("needsApproval"), true);
        return meta;
    }

private slots:
    // Approving clears the gate but does NOT write a fabricated ok/stdout result.
    void approveClearsGateWithoutFabricatingResult() {
        be::DocumentStore doc;
        const be::BlockId id =
            doc.appendTypedBlock(be::BlockType::ToolCall, gatedToolMeta(), nullptr);

        InteractiveTurnHost host(&doc, nullptr);
        QSignalSpy changed(&host, &InteractiveTurnHost::documentChanged);
        host.onApprovalDecided(QStringLiteral("c1"), QStringLiteral("approved"), false);
        QCOMPARE(changed.count(), 1);

        const QVariantMap after = doc.blockAt(doc.rowForBlock(id))->metadata;
        // Gate cleared (shared patch contract)…
        QCOMPARE(after.value(QStringLiteral("approval")).toString(), QStringLiteral("approved"));
        QCOMPARE(after.value(QStringLiteral("needsApproval")).toBool(), false);
        // …but no fabricated completion: no ok status, no synthesized stdout.
        QVERIFY(after.value(QStringLiteral("status")).toString() != QStringLiteral("ok"));
        QVERIFY(!after.contains(QStringLiteral("stdout")));
    }

    // Denying clears the gate and flips to error (the same terminal state the node
    // reaches on a denied action) — still with no fabricated stdout.
    void denyClearsGateWithoutFabricatingStdout() {
        be::DocumentStore doc;
        const be::BlockId id =
            doc.appendTypedBlock(be::BlockType::ToolCall, gatedToolMeta(), nullptr);

        InteractiveTurnHost host(&doc, nullptr);
        host.onApprovalDecided(QStringLiteral("c1"), QStringLiteral("denied"), false);

        const QVariantMap after = doc.blockAt(doc.rowForBlock(id))->metadata;
        QCOMPARE(after.value(QStringLiteral("approval")).toString(), QStringLiteral("denied"));
        QCOMPARE(after.value(QStringLiteral("needsApproval")).toBool(), false);
        QCOMPARE(after.value(QStringLiteral("status")).toString(), QStringLiteral("error"));
        QVERIFY(!after.contains(QStringLiteral("stdout")));
    }
};

QTEST_GUILESS_MAIN(TestInteractiveTurnHost)
#include "tst_interactive_turn_host.moc"
