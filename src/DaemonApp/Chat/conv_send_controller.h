// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror A5 — ConvSend through the durable outbox lane (spec 09 §7 verb seam, §6.4/§6.8,
// §8.4 pending strip). The shared C++ view-model both surfaces bind: the GUI ChatPage's pending
// strip beside the timeline and the TUI chat tab's pending list.
//
// send() enqueues ConvSend to the `chat-send␟transport␟conv` lane — DURABLE before any wire call
// (§6.6 durability point) — and returns; there is NO optimistic mirror row (R2 / ADR-003: pending
// state renders ONLY through the outbox lens, never as a faked ChatMessage). The confirmed line
// appears only when the node's MessagesChanged → ConvHistory delta lands in the mirror window
// (provenance-keyed, uniform §6.6).
//
// Drain is MANUAL against api/38 (§6.8): the outbox holds the lane after reconnect and the surface
// offers a "send" tap. From api/39 the gate opens and the graph drains unattended on reconnect
// (rung 3 dedup makes a resend safe). The owner (app graph) connects the outbox's sendRequested →
// the real ConvSend wire call and reports onAck/onRejection back to the outbox; this controller
// stays a pure lane/pending presenter (no wire).

#include "outbox_types.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace mirror {
class Outbox;
class PendingOpsModel;
} // namespace mirror

class ConvSendController : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // The shared outbox (`Outbox` context property; null in mock mode at M2 — surfaces fall back
    // to the legacy direct send until A8's seeder). Opaque QObject* for QML; qobject_cast inside.
    Q_PROPERTY(QObject* outbox READ outboxObject WRITE setOutboxObject NOTIFY outboxChanged)
    // True when the durable lane is available (outbox bound): the surface routes sends here.
    Q_PROPERTY(bool laneActive READ laneActive NOTIFY outboxChanged)
    Q_PROPERTY(QString transport READ transport NOTIFY conversationChanged)
    Q_PROPERTY(QString conversation READ conversation NOTIFY conversationChanged)
    // The per-conversation pending strip (filtered outbox lens). GUI/TUI render it beside the
    // timeline; QML holds it as QObject* (a QAbstractListModel).
    Q_PROPERTY(QObject* pending READ pending NOTIFY pendingChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingCountChanged)
    Q_PROPERTY(bool lanePaused READ lanePaused NOTIFY lanePausedChanged)

public:
    explicit ConvSendController(QObject* parent = nullptr);

    // Bind the shared outbox (owned by the app graph). Safe to call once.
    void setOutbox(mirror::Outbox* outbox);
    [[nodiscard]] mirror::Outbox* outbox() const;
    [[nodiscard]] QObject* outboxObject() const;
    void setOutboxObject(QObject* outbox);
    [[nodiscard]] bool laneActive() const;

    [[nodiscard]] QString transport() const { return m_transport; }
    [[nodiscard]] QString conversation() const { return m_conversation; }
    [[nodiscard]] QObject* pending() const;
    [[nodiscard]] int pendingCount() const;
    [[nodiscard]] bool lanePaused() const;

    // Bind (or rebind) to a conversation: filters the pending strip to this lane's scope.
    Q_INVOKABLE void open(const QString& transport, const QString& conversation);

    // Enqueue a ConvSend to the chat-send lane. No-op without an outbox / open conversation /
    // non-blank text. Returns the minted op-id ("" on a bound breach — surfaced by the outbox).
    Q_INVOKABLE QString send(const QString& text);

    // Drain (§6.3): a manual tap always; the graph also drains unattended on an api/39 reconnect
    // (§6.8, gated by autoReplayEnabled()).
    Q_INVOKABLE void drain();

    // Rejection affordances (§6.5), one set for every rejected entry.
    Q_INVOKABLE void retry(const QString& opId);
    Q_INVOKABLE void discard(const QString& opId);
    // Reopen a rejected op's text for editing (returns the message body + discards the old op; the
    // surface re-sends via send(), minting a new op-id — the edited-op contract, §6.5).
    Q_INVOKABLE QString takeForEdit(const QString& opId);
    Q_INVOKABLE void sendRemainingAnyway();

    // The auto-replay gate (§6.8): OFF at api/38 (manual drain, "N unsent" prompt), ON from
    // api/39 (unattended reconnect drain). Delegates to mirror::Outbox::autoReplayEnabled.
    [[nodiscard]] Q_INVOKABLE static bool autoReplayEnabled(int apiVersion);

    // The canonical lane scope tail 'transport␟conv' for the bound conversation.
    [[nodiscard]] QString laneScope() const;

signals:
    void outboxChanged();
    void conversationChanged();
    void pendingChanged();
    void pendingCountChanged();
    void lanePausedChanged();
    // A ConvSend was durably enqueued (before any wire call). The surface focuses the pending
    // strip.
    void enqueued(const QString& opId);
    // A ConvSend in THIS conversation's lane was rejected by the node (§6.5): the verb seam's
    // failure signal for the toast/footer notice — the strip row stays the durable surfacing.
    void sendRejected(const QString& message);

private:
    void rebindPendingFilter();

    QPointer<mirror::Outbox> m_outbox;
    mirror::PendingOpsModel* m_pending = nullptr;
    QString m_transport;
    QString m_conversation;
};
