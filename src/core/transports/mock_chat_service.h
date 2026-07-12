// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/ichat_service.h"

#include <QHash>

namespace transports {

// [integrations wire v38] Inert dev stand-in for IChatService: keeps a per-conversation in-memory
// transcript (seeded with a couple of canned messages) so the native chat surface can be built +
// exercised offline (UI-first). send() appends a local echo + re-emits (the mock has no node to be
// authoritative); refreshHistory() re-emits the current set. Replaced by DaemonChatService
// (decoding ConvHistory + forwarding ConvSend) whenever a daemon connects.
class MockChatService : public IChatService {
    Q_OBJECT

public:
    explicit MockChatService(QObject* parent = nullptr);

    [[nodiscard]] QVariantList messages(const QString& transport,
                                        const QString& conv) const override;
    void refreshHistory(const QString& transport, const QString& conv) override;
    void send(const QString& transport, const QString& conv, const QString& text) override;

    // Seed a conversation's transcript (test/dev helper; the mock has no node to fetch from).
    void seed(const QString& transport, const QString& conv, const QVariantList& messages);

private:
    // conv-key ("<transport>\x1f<conv>") -> ordered message rows.
    [[nodiscard]] static QString key(const QString& transport, const QString& conv);
    QHash<QString, QVariantList> m_messages;
};

} // namespace transports
