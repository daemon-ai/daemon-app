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
    // Clear the approval gate (shared patch contract) and stop. The tool's RESULT
    // is never fabricated here: in daemon mode the only completion rendered is the
    // node's real ToolFinished stream event; in mock mode the TurnController emits
    // its own scripted toolFinished from respondApproval. Both arrive through the
    // shared turn-event ingest, not from this host.
    m_doc->updateBlockMetadata(id, be::toolApprovalPatch(decision));
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
