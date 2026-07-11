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

class QAbstractItemModel;

// [integrations wire v38 → mirror M2] Native-chat tab view-model. The single shared
// presentation layer both the QML GUI (ChatPage.qml) and the Tui Widgets TUI
// (RootWidget chat tabs) bind: it owns one conversation's transcript projection
// and the send intent.
//
// READ PATH (M2, spec 09 §8): when a mirror service is bound (daemon mode), the
// timeline reads the mirror's ChatWindowModel — journal-delta row ops off the one
// canonical ChatMessage entity — and open() only DECLARES visibility
// (beginObserving, §5.8); the ingestor owns the top-up fetch. Demand paging rides
// loadEarlier() → olderRequested → Ingestor::requestOlder. Without a mirror (mock
// mode until A8's seeder; substrate-less stacks) the legacy IChatService rows path
// remains: open() asks refreshHistory() and messagesChanged() re-projects.
//
// It re-derives NO domain state — the node owns the transcript; rows are projected
// oldest-first into one markdown document rendered by the EXISTING BlockEditor
// (loadMarkdown). send() (the legacy/mock write path) never echoes locally; in
// mirror mode the surfaces route sends through ConvSendController's outbox lane
// instead (§6.4) and pending intent renders ONLY in the pending strip (R2).
class ChatConversationController : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // The IChatService seam (mock or daemon-backed), bound from QML to the app-global
    // `Chat` context property. Held as QObject* so QML can assign the context property
    // without a registered pointer metatype (mirrors SessionController::store).
    Q_PROPERTY(QObject* service READ service WRITE setService NOTIFY serviceChanged)
    // The mirror composition root (`Mirror` context property; null in mock mode at M2
    // and on substrate-less stacks). Opaque QObject* here — the concrete type is bound
    // in the .cpp behind the substrate define, so this header never forks layouts.
    Q_PROPERTY(QObject* mirror READ mirror WRITE setMirror NOTIFY mirrorChanged)
    Q_PROPERTY(QString transport READ transport NOTIFY conversationChanged)
    Q_PROPERTY(QString conversation READ conversation NOTIFY conversationChanged)
    // The whole transcript projected as one markdown document (oldest-first). The
    // renderers feed this straight into the BlockEditor / DocumentStore.
    Q_PROPERTY(QString markdown READ markdown NOTIFY markdownChanged)
    Q_PROPERTY(int messageCount READ messageCount NOTIFY markdownChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY markdownChanged)
    // Demand paging (§4.6): whether older history may exist (mirror mode; false on the
    // legacy path — v38 loads the whole reachable transcript up front there).
    Q_PROPERTY(bool canLoadEarlier READ canLoadEarlier NOTIFY canLoadEarlierChanged)

public:
    explicit ChatConversationController(QObject* parent = nullptr);
    ~ChatConversationController() override;

    [[nodiscard]] QObject* service() const;
    void setService(QObject* service);
    [[nodiscard]] QObject* mirror() const;
    void setMirror(QObject* mirror);

    [[nodiscard]] QString transport() const { return m_transport; }
    [[nodiscard]] QString conversation() const { return m_conversation; }
    [[nodiscard]] QString markdown() const { return m_markdown; }
    [[nodiscard]] int messageCount() const;
    [[nodiscard]] bool empty() const { return messageCount() == 0; }
    [[nodiscard]] bool canLoadEarlier() const;
    // The mirror timeline lens (a ChatWindowModel; null on the legacy path) — surfaces
    // and tests may bind it directly.
    [[nodiscard]] QAbstractItemModel* timelineModel() const { return m_timeline; }

    // Bind (or rebind) this controller to a conversation. Mirror mode: build the
    // window lens + declare visibility (the ingestor tops up, §5.8). Legacy mode:
    // project any cached rows, then refreshHistory().
    Q_INVOKABLE void open(const QString& transport, const QString& conversation);
    // Send a plain-text line via the LEGACY seam (mock mode / fallback). No local
    // echo. Mirror-mode surfaces route through ConvSendController instead.
    Q_INVOKABLE void send(const QString& text);
    // Demand-page older history (mirror mode; no-op on the legacy path).
    Q_INVOKABLE void loadEarlier();

    // The oldest-first markdown projection of `rows` (a list of IChatService message
    // maps). Static + pure so it is unit-testable and shared by both surfaces.
    [[nodiscard]] static QString projectMarkdown(const QVariantList& rows);

signals:
    void serviceChanged();
    void mirrorChanged();
    void conversationChanged();
    void markdownChanged();
    void canLoadEarlierChanged();
    // A ConvSend for THIS conversation failed on the node/transport (legacy path).
    // The surface shows a non-blocking toast (GUI) / footer notice (TUI).
    void sendFailed(const QString& message);

private:
    // Filtered service handlers: apply only when (transport, conv) matches the bound
    // conversation, so a background tab's controller ignores other conversations.
    void onMessagesChanged(const QString& transport, const QString& conv, const QVariantList& rows);
    void onServiceSendFailed(const QString& transport, const QString& conv, const QString& message);
    // Adopt `rows` as the current transcript and re-project the markdown.
    void applyRows(const QVariantList& rows);
    // Mirror mode internals (bodies live behind the substrate define in the .cpp;
    // no-ops without it).
    void rebindTimeline();
    void teardownTimeline();
    void reprojectFromTimeline();
    [[nodiscard]] bool mirrorActive() const { return m_timeline != nullptr; }

    QPointer<QObject> m_serviceObject;             // the QObject the QML property holds
    transports::IChatService* m_service = nullptr; // qobject_cast of m_serviceObject
    QPointer<QObject> m_mirrorObject;              // MirrorService (opaque at this layer)
    QAbstractItemModel* m_timeline = nullptr;      // owned child; ChatWindowModel in mirror mode
    QString m_transport;
    QString m_conversation;
    QVariantList m_rows;
    QString m_markdown;
};
