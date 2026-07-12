// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

namespace transports {

// [integrations wire v38] Native-chat seam: the client-side view of a conversation's durable
// transcript (ConvHistory) + the send intent (ConvSend), plus the MessagesChanged incremental
// refresh. Backs the native chat tabs (A4): activating a room/DM opens a chat tab that renders
// messages(transport, conv) as a transcript and submits new lines via send(). The node owns the
// transcript (a verifiable per-(transport, conv) journal); the app renders + sends intents, never
// derives message state.
//
// A daemon adapter (DaemonChatService) decodes ConvHistory into ChatMessage rows and forwards
// ConvSend, refetching from its cursor on the MessagesChanged feed event; the mock keeps an
// in-memory transcript so the chat surface can be built + exercised offline (UI-first).
class IChatService : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IChatService() override = default;

    // The last-known messages for a conversation (ConvHistory), oldest-first. Each entry is a
    // display-only map:
    //   id, authorId, authorName, authorIsAgent, replyingTo, text, timestamp, editedAt, error,
    //   title, system, notice, action, highlighted (see DaemonChatService for the projection).
    // `messages()` returns the cached set; `refreshHistory()` triggers a live fetch — the full
    // transcript, paged forward from the head — and fires messagesChanged when it lands. (The v38
    // wire pages forward from a cursor only; there is no backward "load older" op, so the initial
    // refresh fetches the whole transcript.)
    [[nodiscard]] Q_INVOKABLE virtual QVariantList messages(const QString& transport,
                                                            const QString& conv) const {
        Q_UNUSED(transport)
        Q_UNUSED(conv)
        return {};
    }
    Q_INVOKABLE virtual void refreshHistory(const QString& transport, const QString& conv) {
        Q_UNUSED(transport)
        Q_UNUSED(conv)
    }

    // Send a plain-text message to a conversation (ConvSend). On the node's Ok it appends a Chat
    // record + emits MessagesChanged, which the service turns into a messagesChanged refresh — no
    // optimistic local echo (the node is authoritative). Default no-op.
    Q_INVOKABLE virtual void send(const QString& transport, const QString& conv,
                                  const QString& text) {
        Q_UNUSED(transport)
        Q_UNUSED(conv)
        Q_UNUSED(text)
    }

signals:
    // A conversation's transcript changed (a refresh landed / a send round-tripped / the node's
    // MessagesChanged feed event fired). `messages` is the fresh row list (same shape as
    // messages()).
    void messagesChanged(const QString& transport, const QString& conv,
                         const QVariantList& messages);
    // A ConvSend failed on the node or at the transport (surfaced as a toast / TUI notice).
    void sendFailed(const QString& transport, const QString& conv, const QString& message);
};

} // namespace transports
