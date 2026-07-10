// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>

namespace transports {
class IChatService;
}

// [integrations wire v38] Native-chat tab view-model (A4). The single shared
// presentation layer both the QML GUI (ChatPage.qml) and the Tui Widgets TUI
// (RootWidget chat tabs) bind: it owns one conversation's transcript projection
// and the send intent, over the IChatService seam.
//
// It re-derives NO domain state — the node owns the transcript. On open() it asks
// the service to refreshHistory(); the authoritative rows arrive via
// messagesChanged() (driven by the node's MessagesChanged event) and are projected
// oldest-first into a single markdown document rendered by the EXISTING BlockEditor
// (loadMarkdown). send() forwards to the service and never echoes locally — the sent
// line appears only when the node's own MessagesChanged round-trips back. sendFailed
// surfaces as a non-blocking notice per the house pattern.
//
// The controller filters the service's per-(transport, conversation) signals to its
// own bound conversation, so a backgrounded chat tab keeps its transcript current
// while another tab is foregrounded.
class ChatConversationController : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // The IChatService seam (mock or daemon-backed), bound from QML to the app-global
    // `Chat` context property. Held as QObject* so QML can assign the context property
    // without a registered pointer metatype (mirrors SessionController::store).
    Q_PROPERTY(QObject* service READ service WRITE setService NOTIFY serviceChanged)
    Q_PROPERTY(QString transport READ transport NOTIFY conversationChanged)
    Q_PROPERTY(QString conversation READ conversation NOTIFY conversationChanged)
    // The whole transcript projected as one markdown document (oldest-first). The
    // renderers feed this straight into the BlockEditor / DocumentStore.
    Q_PROPERTY(QString markdown READ markdown NOTIFY markdownChanged)
    Q_PROPERTY(int messageCount READ messageCount NOTIFY markdownChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY markdownChanged)

public:
    explicit ChatConversationController(QObject* parent = nullptr);

    [[nodiscard]] QObject* service() const;
    void setService(QObject* service);

    [[nodiscard]] QString transport() const { return m_transport; }
    [[nodiscard]] QString conversation() const { return m_conversation; }
    [[nodiscard]] QString markdown() const { return m_markdown; }
    [[nodiscard]] int messageCount() const { return static_cast<int>(m_rows.size()); }
    [[nodiscard]] bool empty() const { return m_rows.isEmpty(); }

    // Bind (or rebind) this controller to a conversation: adopt (transport,
    // conversation), project any already-cached rows immediately, then ask the
    // service for the full transcript (refreshHistory). Rebinding to a new
    // conversation clears the prior projection first.
    Q_INVOKABLE void open(const QString& transport, const QString& conversation);
    // Send a plain-text line to the bound conversation (ConvSend). No local echo —
    // the authoritative row lands via messagesChanged. No-op without a service /
    // an open conversation / non-blank text.
    Q_INVOKABLE void send(const QString& text);

    // The oldest-first markdown projection of `rows` (a list of IChatService message
    // maps). Static + pure so it is unit-testable and shared by both surfaces.
    [[nodiscard]] static QString projectMarkdown(const QVariantList& rows);

signals:
    void serviceChanged();
    void conversationChanged();
    void markdownChanged();
    // A ConvSend for THIS conversation failed on the node/transport. The surface
    // shows a non-blocking toast (GUI) / footer notice (TUI).
    void sendFailed(const QString& message);

private:
    // Filtered service handlers: apply only when (transport, conv) matches the bound
    // conversation, so a background tab's controller ignores other conversations.
    void onMessagesChanged(const QString& transport, const QString& conv, const QVariantList& rows);
    void onServiceSendFailed(const QString& transport, const QString& conv, const QString& message);
    // Adopt `rows` as the current transcript and re-project the markdown.
    void applyRows(const QVariantList& rows);

    QPointer<QObject> m_serviceObject;             // the QObject the QML property holds
    transports::IChatService* m_service = nullptr; // qobject_cast of m_serviceObject
    QString m_transport;
    QString m_conversation;
    QVariantList m_rows;
    QString m_markdown;
};
