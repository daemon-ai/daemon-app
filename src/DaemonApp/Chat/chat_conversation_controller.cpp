// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "chat_conversation_controller.h"

#include "mirror/chat_window_model.h"
#include "mirror/mirror_service.h"
#include "transports/ichat_service.h"

#include <QAbstractItemModel>
#include <QChar>
#include <QStringList>

namespace {

QString fieldText(const QVariantMap& row, const char* key) {
    return row.value(QLatin1String(key)).toString();
}

bool fieldFlag(const QVariantMap& row, const char* key) {
    return row.value(QLatin1String(key)).toBool();
}

} // namespace

ChatConversationController::ChatConversationController(QObject* parent) : QObject(parent) {}

ChatConversationController::~ChatConversationController() {
    teardownTimeline();
}

QObject* ChatConversationController::service() const {
    return m_serviceObject;
}

void ChatConversationController::setService(QObject* service) {
    if (m_serviceObject == service) {
        return;
    }
    if (m_service != nullptr) {
        disconnect(m_service, nullptr, this, nullptr);
    }
    m_serviceObject = service;
    m_service = qobject_cast<transports::IChatService*>(service);
    if (m_service != nullptr) {
        connect(m_service, &transports::IChatService::messagesChanged, this,
                &ChatConversationController::onMessagesChanged);
        connect(m_service, &transports::IChatService::sendFailed, this,
                &ChatConversationController::onServiceSendFailed);
    }
    emit serviceChanged();
}

QObject* ChatConversationController::mirror() const {
    return m_mirrorObject;
}

void ChatConversationController::setMirror(QObject* mirror) {
    if (m_mirrorObject == mirror) {
        return;
    }
    m_mirrorObject = mirror;
    // Rebind the read path for the current conversation (if any) onto/off the mirror.
    if (!m_transport.isEmpty()) {
        rebindTimeline();
    }
    emit mirrorChanged();
}

int ChatConversationController::messageCount() const {
    if (m_timeline != nullptr) {
        return m_timeline->rowCount();
    }
    return static_cast<int>(m_rows.size());
}

void ChatConversationController::open(const QString& transport, const QString& conversation) {
    const bool changed = m_transport != transport || m_conversation != conversation;
    if (changed) {
        m_transport = transport;
        m_conversation = conversation;
        emit conversationChanged();
    }
    rebindTimeline();
    if (mirrorActive()) {
        // Mirror read path (M2): the window lens + the visibility declaration made in
        // rebindTimeline() are everything — the INGESTOR owns the top-up fetch (§5.8).
        return;
    }
    if (changed) {
        // Legacy path: rebinding must not leak the prior transcript — clear first, then project
        // whatever the service already has cached.
        applyRows(m_service != nullptr ? m_service->messages(transport, conversation)
                                       : QVariantList());
    }
    // Ask the node for the transcript (v38 pages forward-only, so the initial fetch is the whole
    // history). The authoritative rows land via messagesChanged and re-project.
    if (m_service != nullptr) {
        m_service->refreshHistory(transport, conversation);
    }
}

void ChatConversationController::send(const QString& text) {
    const QString trimmed = text.trimmed();
    if (m_service == nullptr || m_transport.isEmpty() || trimmed.isEmpty()) {
        return;
    }
    // No optimistic local echo — the node is authoritative. The sent line reappears
    // when the resulting MessagesChanged round-trips back through onMessagesChanged.
    m_service->send(m_transport, m_conversation, text);
}

void ChatConversationController::onMessagesChanged(const QString& transport, const QString& conv,
                                                   const QVariantList& rows) {
    if (mirrorActive()) {
        return; // the mirror window is the read path; legacy rows are dual-write bookkeeping
    }
    // Filter to the bound conversation so a backgrounded tab's controller ignores
    // other conversations' traffic.
    if (transport != m_transport || conv != m_conversation) {
        return;
    }
    applyRows(rows);
}

void ChatConversationController::onServiceSendFailed(const QString& transport, const QString& conv,
                                                     const QString& message) {
    if (transport != m_transport || conv != m_conversation) {
        return;
    }
    emit sendFailed(message);
}

void ChatConversationController::applyRows(const QVariantList& rows) {
    m_rows = rows;
    m_markdown = projectMarkdown(rows);
    emit markdownChanged();
}

QString ChatConversationController::projectMarkdown(const QVariantList& rows) {
    // Oldest-first: each row becomes one or more markdown blocks the BlockEditor
    // renders. Messages emphasise their author; system/notice events and /me actions
    // render as distinct notes so the transcript styles events vs. lines.
    QStringList blocks;
    blocks.reserve(rows.size());
    for (const QVariant& v : rows) {
        const QVariantMap row = v.toMap();
        const QString text = fieldText(row, "text");
        const QString author = fieldText(row, "authorName");
        if (fieldFlag(row, "system") || fieldFlag(row, "notice")) {
            // A room event / ephemeral notice: an italic blockquote note.
            blocks.append(QStringLiteral("> _%1_").arg(text));
            continue;
        }
        if (fieldFlag(row, "action")) {
            // An IRC-style "/me" action line.
            blocks.append(QStringLiteral("_%1 %2_").arg(author, text));
            continue;
        }
        // A normal message: bold author header, then the markdown body. Failed sends
        // get a leading warning glyph (no translatable string, so the projection
        // stays i18n-free).
        const QString body = fieldFlag(row, "error") ? (QStringLiteral("\u26a0 ") + text) : text;
        blocks.append(QStringLiteral("**%1**\n\n%2").arg(author, body));
    }
    return blocks.join(QStringLiteral("\n\n"));
}

// ---------------------------------------------------------------------------
// Mirror read path (M2 → AD): unconditional — the mirror is the only data path.
// ---------------------------------------------------------------------------

bool ChatConversationController::canLoadEarlier() const {
    const auto* wm = static_cast<const mirror::ChatWindowModel*>(m_timeline);
    return wm != nullptr && wm->hasMoreOlder();
}

void ChatConversationController::loadEarlier() {
    if (auto* wm = static_cast<mirror::ChatWindowModel*>(m_timeline)) {
        wm->requestOlder(0); // 0 = the model's default page size
    }
}

void ChatConversationController::rebindTimeline() {
    teardownTimeline();
    auto* svc = qobject_cast<mirror::MirrorService*>(m_mirrorObject);
    if (svc == nullptr || m_transport.isEmpty()) {
        reprojectFromTimeline(); // legacy markdown stays whatever applyRows() set
        return;
    }
    const mirror::ChatMessageScope scope{m_transport, m_conversation};
    auto* timeline = new mirror::ChatWindowModel(svc->store(), scope, this);
    m_timeline = timeline;

    // Journal-delta row ops → one markdown re-projection per change wave (§8.1).
    const auto reproject = [this] { reprojectFromTimeline(); };
    connect(timeline, &QAbstractItemModel::rowsInserted, this, reproject);
    connect(timeline, &QAbstractItemModel::rowsRemoved, this, reproject);
    connect(timeline, &QAbstractItemModel::rowsMoved, this, reproject);
    connect(timeline, &QAbstractItemModel::dataChanged, this, reproject);
    connect(timeline, &QAbstractItemModel::modelReset, this, reproject);

    // Demand-paging mediation (§4.6): the lens declares intent; the ingestor decides. count == 0
    // is WindowModel's has-more wake signal, not a request — surface it as a property change.
    connect(timeline, &mirror::TableModelBase::olderRequested, this,
            [this, svc](const QString& scopeKey, int count) {
                if (count <= 0) {
                    emit canLoadEarlierChanged();
                    return;
                }
                if (auto* wm = static_cast<mirror::ChatWindowModel*>(m_timeline)) {
                    if (!svc->ingestor().requestOlder(scopeKey, count)) {
                        wm->setHasMoreOlder(false); // end-of-history until BR's before_cursor
                    }
                }
            });

    // Visibility declaration (§5.8): the ingestor tops the window up from the stored cursor.
    svc->ingestor().beginObserving(QStringLiteral("chat"), scope.serialize());

    reprojectFromTimeline();
    emit canLoadEarlierChanged();
}

void ChatConversationController::teardownTimeline() {
    if (m_timeline == nullptr) {
        return;
    }
    if (auto* svc = qobject_cast<mirror::MirrorService*>(m_mirrorObject)) {
        const auto* wm = static_cast<const mirror::ChatWindowModel*>(m_timeline);
        svc->ingestor().endObserving(QStringLiteral("chat"), wm->scope().serialize());
    }
    m_timeline->deleteLater();
    m_timeline = nullptr;
}

void ChatConversationController::reprojectFromTimeline() {
    if (m_timeline == nullptr) {
        return;
    }
    // Project the canonical ChatMessage roles into the same oldest-first markdown the BlockEditor
    // renders on both surfaces. The canonical entity carries author/text/error/edited (§3.1); the
    // legacy system/notice/action styling flags are not entity fields — a G-series entity-map
    // extension, documented in the ledger.
    QStringList blocks;
    const int n = m_timeline->rowCount();
    blocks.reserve(n);
    for (int i = 0; i < n; ++i) {
        const QModelIndex idx = m_timeline->index(i, 0);
        const QString author =
            m_timeline->data(idx, mirror::ChatWindowModel::AuthorRole).toString();
        const QString text = m_timeline->data(idx, mirror::ChatWindowModel::TextRole).toString();
        const QString error = m_timeline->data(idx, mirror::ChatWindowModel::ErrorRole).toString();
        const QString body = !error.isEmpty() ? (QStringLiteral("\u26a0 ") + text) : text;
        blocks.append(QStringLiteral("**%1**\n\n%2").arg(author, body));
    }
    m_markdown = blocks.join(QStringLiteral("\n\n"));
    emit markdownChanged();
}
