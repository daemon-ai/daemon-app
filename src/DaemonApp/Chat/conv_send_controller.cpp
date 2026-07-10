// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "conv_send_controller.h"

#include "outbox.h"
#include "pending_ops_model.h"
#include "verb_class.h"

#include <QChar>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
// The lane scope for a conversation: 'transport␟conv' (unit separator, §3.1 canonical key).
QString scopeTail(const QString& transport, const QString& conv) {
    return transport + QChar(0x1f) + conv;
}
} // namespace

ConvSendController::ConvSendController(QObject* parent)
    : QObject(parent), m_pending(new mirror::PendingOpsModel(this)) {}

void ConvSendController::setOutbox(mirror::Outbox* outbox) {
    if (m_outbox == outbox) {
        return;
    }
    if (m_outbox != nullptr) {
        disconnect(m_outbox, nullptr, this, nullptr);
    }
    m_outbox = outbox;
    m_pending->setOutbox(outbox);
    if (m_outbox != nullptr) {
        // Surface counts + pause state changing so the strip re-renders reactively.
        connect(m_outbox, &mirror::Outbox::opAdded, this, [this] { emit pendingCountChanged(); });
        connect(m_outbox, &mirror::Outbox::opRemoved, this, [this] { emit pendingCountChanged(); });
        connect(m_outbox, &mirror::Outbox::lanePausedChanged, this,
                [this](const QString& lane, bool) {
                    if (lane == mirror::buildLane(mirror::VerbClass::ChatSend, laneScope())) {
                        emit lanePausedChanged();
                    }
                });
    }
    rebindPendingFilter();
    emit pendingChanged();
}

mirror::Outbox* ConvSendController::outbox() const {
    return m_outbox.data();
}

QObject* ConvSendController::pending() const {
    return m_pending;
}

int ConvSendController::pendingCount() const {
    return m_pending->count();
}

bool ConvSendController::lanePaused() const {
    if (m_outbox == nullptr || m_transport.isEmpty()) {
        return false;
    }
    return m_outbox->lanePaused(mirror::buildLane(mirror::VerbClass::ChatSend, laneScope()));
}

QString ConvSendController::laneScope() const {
    return scopeTail(m_transport, m_conversation);
}

void ConvSendController::open(const QString& transport, const QString& conversation) {
    if (m_transport == transport && m_conversation == conversation) {
        return;
    }
    m_transport = transport;
    m_conversation = conversation;
    rebindPendingFilter();
    emit conversationChanged();
    emit pendingCountChanged();
    emit lanePausedChanged();
}

void ConvSendController::rebindPendingFilter() {
    // Filter the shared outbox lens to THIS conversation's chat-send lane scope (§8.4).
    m_pending->setScopeFilter(laneScope());
}

QString ConvSendController::send(const QString& text) {
    const QString trimmed = text.trimmed();
    if (m_outbox == nullptr || m_transport.isEmpty() || trimmed.isEmpty()) {
        return {};
    }
    // Canonical request args (op_id is added by the outbox row / minted here). The owner decodes
    // this payload to build the wire ConvSend when the lane drains.
    QJsonObject args;
    args.insert(QStringLiteral("transport"), m_transport);
    args.insert(QStringLiteral("conv"), m_conversation);
    args.insert(QStringLiteral("message"), trimmed);
    const QByteArray payload = QJsonDocument(args).toJson(QJsonDocument::Compact);

    const QString opId = m_outbox->enqueue(QStringLiteral("ConvSend"), laneScope(), payload, trimmed);
    if (!opId.isEmpty()) {
        emit enqueued(opId);
    }
    return opId;
}

void ConvSendController::drain() {
    if (m_outbox != nullptr) {
        m_outbox->drain(); // manual only; never auto (§6.8)
    }
}

void ConvSendController::retry(const QString& opId) {
    if (m_outbox != nullptr) {
        m_outbox->retry(opId);
    }
}

void ConvSendController::discard(const QString& opId) {
    if (m_outbox != nullptr) {
        m_outbox->discard(opId);
    }
}

QString ConvSendController::takeForEdit(const QString& opId) {
    if (m_outbox == nullptr) {
        return {};
    }
    const QByteArray payload = m_outbox->takeForEdit(opId);
    const QJsonObject args = QJsonDocument::fromJson(payload).object();
    return args.value(QStringLiteral("message")).toString();
}

void ConvSendController::sendRemainingAnyway() {
    if (m_outbox != nullptr && !m_transport.isEmpty()) {
        m_outbox->sendRemainingAnyway(mirror::buildLane(mirror::VerbClass::ChatSend, laneScope()));
    }
}

bool ConvSendController::autoReplayEnabled(int apiVersion) {
    return mirror::Outbox::autoReplayEnabled(apiVersion);
}
