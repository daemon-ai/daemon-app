// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror::ChatWindowModel — the M2 chat-timeline read lens (spec 09 §8.1/§8.3, §4.6). The
// concrete WindowModel<ChatMessage> the chat surface binds: cursor-ordered messages for one
// (transport, conv) window, exact row ops off the journal delta, demand-paging older history via
// olderRequested (§4.6 — the ingestor fulfils the backward/forward fetch). Curated role map reads
// typed entity fields straight off the generated ChatMessage Q_GADGET — no QVariantMap shape, the
// single canonical form retiring the 2 chat-message layouts (07§6.9). GUI-free C++ (§8.4): QML
// binds it, the TUI consumes it via DisplayRoleAdapter. Pending sends never appear here (R2) —
// the surface composes this window model + the outbox pending strip as two sources.

#include "mirror/generated/entities_gen.h"
#include "mirror/window_model.h"

#include <QByteArray>
#include <QHash>
#include <QVariant>

namespace mirror {

class ChatWindowModel : public WindowModel<ChatMessage> {
    Q_OBJECT

public:
    enum Roles : int {
        TransportRole = Qt::UserRole + 100,
        ConversationRole,
        CursorRole,
        AuthorRole,
        TextRole,
        TimestampRole,
        EditedAtRole,
        ReplyingToRole,
        ErrorRole,
        AttachmentCountRole,
        OriginOpRole,
    };

    ChatWindowModel(Store& store, ChatMessageScope scope, QObject* parent = nullptr);

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

protected:
    [[nodiscard]] QVariant roleData(const ChatMessage& m, int role) const override;
};

} // namespace mirror
