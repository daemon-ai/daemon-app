// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/transcript_projection.h"

#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace daemonapp::daemon {

// A7T/G2: the msg-fence transcript projection over mirror `TranscriptBlock` window items (seq
// order). Ported grammar-for-grammar from `CachedSessionStore::content()` so a re-projection is
// byte-stable and byte-IDENTICAL to the legacy cache projection. G2 (M5) enriched the entity with
// `args_summary` + `detail_kind`/`detail_body`, closing the A7T "ENTITY-FIELD GAP": tool fences
// with args and structured results now reproduce exactly (S7/S8 parity).
QString projectTranscriptBlocks(const std::vector<mirror::TranscriptBlock>& blocks) {
    // Pre-pass: the `todo` tool's blocks are the composer status stack's feed, not transcript
    // content (mirrors the engine's live suppression) — collect its call ids so results skip too.
    // Every other ToolResult is folded into its ToolCall's fence (ok/error status).
    QSet<QString> todoCalls;
    QHash<QString, const mirror::TranscriptBlock*> resultByCall;
    for (const mirror::TranscriptBlock& b : blocks) {
        if (b.kind == QStringLiteral("ToolCall") && b.name == QStringLiteral("todo")) {
            todoCalls.insert(b.call_id);
        } else if (b.kind == QStringLiteral("ToolResult") && !resultByCall.contains(b.call_id)) {
            resultByCall.insert(b.call_id, &b);
        }
    }

    QString out;
    QString openRole;
    bool openedByAgentBlock = false;
    const auto openMessage = [&out, &openRole](const QString& role, quint64 seq) {
        openRole = role;
        out += QStringLiteral("```msg\n{\"id\":\"cache-%1\",\"role\":\"%2\"}\n```\n\n")
                   .arg(seq)
                   .arg(role);
    };
    const auto openAssistantFor = [&openMessage, &openRole, &openedByAgentBlock](quint64 seq) {
        if (openRole != QStringLiteral("assistant")) {
            openMessage(QStringLiteral("assistant"), seq);
            openedByAgentBlock = true;
        }
    };
    const auto appendFence = [&out](const QString& info, const QJsonObject& meta) {
        out +=
            QStringLiteral("```%1\n%2\n```\n\n")
                .arg(info, QString::fromUtf8(QJsonDocument(meta).toJson(QJsonDocument::Compact)));
    };
    for (const mirror::TranscriptBlock& b : blocks) {
        if (b.kind == QStringLiteral("Message")) {
            const QString role =
                (b.role.isEmpty() ? QStringLiteral("Assistant") : b.role).toLower();
            const bool continuesAgentTurn =
                openedByAgentBlock && role == QStringLiteral("assistant") && openRole == role;
            if (!continuesAgentTurn) {
                openMessage(role, b.seq);
            }
            openedByAgentBlock = false;
            out += b.text;
            out += QStringLiteral("\n\n");
        } else if (b.kind == QStringLiteral("Reasoning")) {
            openAssistantFor(b.seq);
            appendFence(QStringLiteral("reasoning"),
                        QJsonObject{{QStringLiteral("status"), QStringLiteral("complete")},
                                    {QStringLiteral("body"), b.text}});
        } else if (b.kind == QStringLiteral("ToolCall")) {
            if (todoCalls.contains(b.call_id)) {
                continue;
            }
            openAssistantFor(b.seq);
            const mirror::TranscriptBlock* result = resultByCall.value(b.call_id, nullptr);
            const QString status = result == nullptr ? QStringLiteral("running")
                                   : result->ok      ? QStringLiteral("ok")
                                                     : QStringLiteral("error");
            // The ToolCall row carries the args key the legacy fence always emits (G2 entity gap
            // closed); QJsonObject sorts keys alphabetically, matching the legacy compact JSON.
            QJsonObject tool{{QStringLiteral("callId"), b.call_id},
                             {QStringLiteral("name"), b.name},
                             {QStringLiteral("argsSummary"), b.args_summary},
                             {QStringLiteral("status"), status}};
            // Fold the result: full content in `summary`, typed detail (kind + UTF-8 body) — the
            // exact shape (and order) the legacy CachedSessionStore::content() emitted (D1).
            if (result != nullptr) {
                if (!result->summary.isEmpty()) {
                    tool.insert(QStringLiteral("summary"), result->summary);
                }
                if (!result->detail_kind.isEmpty()) {
                    tool.insert(QStringLiteral("detailKind"), result->detail_kind);
                    tool.insert(QStringLiteral("detailBody"), result->detail_body);
                }
            }
            appendFence(QStringLiteral("tool"), tool);
        }
        // ToolResult rows carry no standalone rendering: folded into their call's fence above.
    }
    return out.trimmed();
}

} // namespace daemonapp::daemon
