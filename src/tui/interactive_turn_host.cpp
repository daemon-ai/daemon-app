// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "interactive_turn_host.h"

#include "core/agent_block.h"
#include "core/agent_ingest.h"
#include "core/block_record.h"
#include "core/document_store.h"

#include <QMetaType>
#include <QStringList>
#include <QVariantList>

InteractiveTurnHost::InteractiveTurnHost(be::DocumentStore* doc, be::TranscriptIngest* ingest,
                                         QObject* parent)
    : QObject(parent), m_doc(doc), m_ingest(ingest) {}

void InteractiveTurnHost::onApprovalDecided(const QString& callId, const QString& decision,
                                            bool /*permanent*/) {
    if (m_doc == nullptr) {
        return;
    }
    const be::BlockId id = m_doc->blockIdForMetadata(QStringLiteral("callId"), callId);
    if (id == 0) {
        return;
    }
    // Clear the approval gate (shared patch contract).
    m_doc->updateBlockMetadata(id, be::toolApprovalPatch(decision));

    // An approval drives the dangerous tool to a finished ok state, mirroring the
    // QML mock host (a denial leaves it in the error state toolApprovalPatch set).
    if (decision == QStringLiteral("approved")) {
        QVariantMap done;
        done.insert(QStringLiteral("status"), QStringLiteral("ok"));
        done.insert(QStringLiteral("durationMs"), 1400);
        done.insert(QStringLiteral("detailKind"), QStringLiteral("ansi-stream"));
        done.insert(QStringLiteral("stdout"),
                    tr("\u001b[32m\u2713\u001b[0m approved \u2014 command finished\n"));
        m_doc->updateBlockMetadata(id, done);
    }
    emit documentChanged();
}

void InteractiveTurnHost::onClarifySubmitted(const QString& callId, const QString& /*requestId*/,
                                             const QVariantMap& answers) {
    if (m_doc == nullptr) {
        return;
    }
    const be::BlockId id = m_doc->blockIdForMetadata(QStringLiteral("callId"), callId);
    if (id == 0) {
        return;
    }
    // Mark the clarify answered (shared patch contract).
    m_doc->updateBlockMetadata(id, be::clarifyAnswerPatch(answers));

    // Stream a short follow-up so the turn visibly continues, mirroring the mock
    // host: flatten the answers to a readable summary.
    QStringList parts;
    for (auto it = answers.cbegin(); it != answers.cend(); ++it) {
        if (it.value().typeId() == QMetaType::QVariantList) {
            parts << it.value().toStringList().join(QStringLiteral(", "));
        } else {
            parts << it.value().toString();
        }
    }
    if (m_ingest != nullptr) {
        QVariantMap text;
        text.insert(QStringLiteral("type"), QStringLiteral("text"));
        text.insert(
            QStringLiteral("text"),
            tr("\n\nThanks \u2014 proceeding with: %1\n").arg(parts.join(QStringLiteral("; "))));
        QVariantMap flush;
        flush.insert(QStringLiteral("type"), QStringLiteral("flush"));
        m_ingest->ingestAll(QVariantList{text, flush});
    }
    emit documentChanged();
}
