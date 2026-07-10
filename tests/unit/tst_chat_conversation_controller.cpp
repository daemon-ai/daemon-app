// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [integrations wire v38] Work package A4 (app/chat-tabs): the shared native-chat
// view-model. ChatConversationController is the single presentation layer BOTH the
// GUI (ChatPage.qml) and the TUI (RootWidget chat tabs) bind — so this guards its
// contract independent of either surface: history load on open, the oldest-first
// markdown projection of ConvHistory rows, send-without-local-echo, authoritative
// MessagesChanged refresh application (foreground AND background), per-conversation
// signal filtering, and sendFailed surfacing. Driven against MockChatService plus a
// controllable recording double (to assert the no-echo seam precisely).

#include "chat_conversation_controller.h"
#include "transports/ichat_service.h"
#include "transports/mock_chat_service.h"

#include <QSignalSpy>
#include <QtTest>

using transports::MockChatService;

namespace {

// A test double with full control over emissions: records send()/refreshHistory()
// calls and emits messagesChanged/sendFailed only when the test drives it. Unlike
// MockChatService (which echoes on send), it never emits on its own — so a test can
// prove the CONTROLLER performs no local echo (the row appears only when the node's
// authoritative MessagesChanged round-trips).
class RecordingChatService : public transports::IChatService {
    Q_OBJECT

public:
    using IChatService::IChatService;

    [[nodiscard]] QVariantList messages(const QString& transport,
                                        const QString& conv) const override {
        return m_store.value(key(transport, conv));
    }
    void refreshHistory(const QString& transport, const QString& conv) override {
        refreshCalls.append(qMakePair(transport, conv));
    }
    void send(const QString& transport, const QString& conv, const QString& text) override {
        sendCalls.append(std::make_tuple(transport, conv, text));
    }

    // Test driver: replace the stored rows and fire the authoritative feed event.
    void deliver(const QString& transport, const QString& conv, const QVariantList& rows) {
        m_store.insert(key(transport, conv), rows);
        emit messagesChanged(transport, conv, rows);
    }
    void fail(const QString& transport, const QString& conv, const QString& message) {
        emit sendFailed(transport, conv, message);
    }

    QList<QPair<QString, QString>> refreshCalls;
    QList<std::tuple<QString, QString, QString>> sendCalls;

private:
    [[nodiscard]] static QString key(const QString& transport, const QString& conv) {
        return transport + QChar(0x1f) + conv;
    }
    QHash<QString, QVariantList> m_store;
};

// One transcript row in the IChatService display shape.
QVariantMap msg(const QString& author, const QString& text, bool system = false,
                bool notice = false) {
    QVariantMap m;
    m[QStringLiteral("authorName")] = author;
    m[QStringLiteral("authorId")] = author;
    m[QStringLiteral("text")] = text;
    m[QStringLiteral("system")] = system;
    m[QStringLiteral("notice")] = notice;
    m[QStringLiteral("action")] = false;
    m[QStringLiteral("error")] = false;
    return m;
}

const QString kT = QStringLiteral("demo/acct");
const QString kC = QStringLiteral("!room:demo");

} // namespace

class TestChatConversationController : public QObject {
    Q_OBJECT

private slots:
    // open() adopts (transport, conversation) and asks the service for the full
    // transcript (the v38 wire pages forward-only, so the initial fetch is the whole
    // history).
    void openRefreshesHistory() {
        RecordingChatService svc;
        ChatConversationController convo;
        convo.setService(&svc);

        convo.open(kT, kC);
        QCOMPARE(convo.transport(), kT);
        QCOMPARE(convo.conversation(), kC);
        QCOMPARE(svc.refreshCalls.size(), 1);
        QCOMPARE(svc.refreshCalls.first().first, kT);
        QCOMPARE(svc.refreshCalls.first().second, kC);
    }

    // The authoritative rows (delivered via messagesChanged) project oldest-first
    // into one markdown document, carrying each author + message body in order.
    void projectsOldestFirstMarkdown() {
        MockChatService svc;
        svc.seed(kT, kC,
                 {msg(QStringLiteral("Alice"), QStringLiteral("hello")),
                  msg(QStringLiteral("Bob"), QStringLiteral("hi there"))});
        ChatConversationController convo;
        convo.setService(&svc);

        QSignalSpy md(&convo, &ChatConversationController::markdownChanged);
        convo.open(kT, kC);
        QVERIFY(md.count() >= 1);
        QCOMPARE(convo.messageCount(), 2);
        const QString out = convo.markdown();
        QVERIFY2(out.contains(QStringLiteral("Alice")), qPrintable(out));
        QVERIFY2(out.contains(QStringLiteral("hello")), qPrintable(out));
        QVERIFY2(out.contains(QStringLiteral("Bob")), qPrintable(out));
        QVERIFY2(out.contains(QStringLiteral("hi there")), qPrintable(out));
        // Oldest-first: Alice's line precedes Bob's.
        QVERIFY(out.indexOf(QStringLiteral("hello")) < out.indexOf(QStringLiteral("hi there")));
    }

    // A system/notice row renders as a distinct note (no bold author header), while
    // a normal row gets its author emphasised — so the transcript styles events vs
    // messages. Guards the projection shape without over-fitting exact syntax.
    void systemAndNoticeRowsRenderDistinctly() {
        MockChatService svc;
        svc.seed(
            kT, kC,
            {msg(QStringLiteral("system"), QStringLiteral("Alice joined the room"), true, false),
             msg(QStringLiteral("Bob"), QStringLiteral("welcome"))});
        ChatConversationController convo;
        convo.setService(&svc);
        convo.open(kT, kC);

        const QString out = convo.markdown();
        // The normal message emphasises its author.
        QVERIFY2(out.contains(QStringLiteral("**Bob**")), qPrintable(out));
        // The system event does NOT get a bold author header for "system".
        QVERIFY2(!out.contains(QStringLiteral("**system**")), qPrintable(out));
        // Its text is still present (rendered as a note).
        QVERIFY2(out.contains(QStringLiteral("Alice joined the room")), qPrintable(out));
    }

    // send() forwards to the service and performs NO optimistic local echo: the
    // transcript is unchanged until the node's authoritative MessagesChanged lands.
    void sendDoesNotEchoLocally() {
        RecordingChatService svc;
        ChatConversationController convo;
        convo.setService(&svc);
        convo.open(kT, kC);
        QVERIFY(convo.empty());

        convo.send(QStringLiteral("hello from me"));
        // Forwarded to ConvSend...
        QCOMPARE(svc.sendCalls.size(), 1);
        QCOMPARE(std::get<2>(svc.sendCalls.first()), QStringLiteral("hello from me"));
        // ...but NOT echoed into the transcript.
        QVERIFY(convo.empty());
        QVERIFY(!convo.markdown().contains(QStringLiteral("hello from me")));

        // The node echoes it back authoritatively -> now it appears (exactly once).
        svc.deliver(kT, kC, {msg(QStringLiteral("Me"), QStringLiteral("hello from me"))});
        QCOMPARE(convo.messageCount(), 1);
        const QString out = convo.markdown();
        QCOMPARE(out.count(QStringLiteral("hello from me")), 1);
    }

    // A MessagesChanged refresh replaces the transcript with the authoritative rows
    // (the node is the source of truth — the client never merges/derives).
    void authoritativeRefreshReplacesRows() {
        RecordingChatService svc;
        ChatConversationController convo;
        convo.setService(&svc);
        convo.open(kT, kC);

        svc.deliver(kT, kC, {msg(QStringLiteral("Bob"), QStringLiteral("one"))});
        QCOMPARE(convo.messageCount(), 1);
        svc.deliver(kT, kC,
                    {msg(QStringLiteral("Bob"), QStringLiteral("one")),
                     msg(QStringLiteral("Bob"), QStringLiteral("two"))});
        QCOMPARE(convo.messageCount(), 2);
        QVERIFY(convo.markdown().contains(QStringLiteral("two")));
    }

    // The controller filters the service's per-conversation signals: a
    // MessagesChanged / sendFailed for a DIFFERENT conversation is ignored, so a
    // backgrounded tab's controller never picks up another conversation's traffic.
    void ignoresOtherConversations() {
        RecordingChatService svc;
        ChatConversationController convo;
        convo.setService(&svc);
        convo.open(kT, kC);

        QSignalSpy failed(&convo, &ChatConversationController::sendFailed);
        svc.deliver(kT, QStringLiteral("!other:demo"),
                    {msg(QStringLiteral("Zed"), QStringLiteral("elsewhere"))});
        svc.fail(kT, QStringLiteral("!other:demo"), QStringLiteral("nope"));
        QCOMPARE(convo.messageCount(), 0);
        QVERIFY(convo.empty());
        QCOMPARE(failed.count(), 0);
    }

    // A backgrounded conversation (never re-opened) still applies MessagesChanged —
    // the controller is signal-driven, not gated on being the foreground tab.
    void backgroundConversationStillUpdates() {
        RecordingChatService svc;
        ChatConversationController convo;
        convo.setService(&svc);
        convo.open(kT, kC);
        // Simulate being backgrounded: no further open()/interaction, just a feed
        // event for the bound conversation.
        svc.deliver(kT, kC, {msg(QStringLiteral("Bob"), QStringLiteral("while away"))});
        QCOMPARE(convo.messageCount(), 1);
        QVERIFY(convo.markdown().contains(QStringLiteral("while away")));
    }

    // A ConvSend failure for the bound conversation is surfaced (scoped, non-blocking).
    void sendFailedForwardedForBoundConversation() {
        RecordingChatService svc;
        ChatConversationController convo;
        convo.setService(&svc);
        convo.open(kT, kC);

        QSignalSpy failed(&convo, &ChatConversationController::sendFailed);
        svc.fail(kT, kC, QStringLiteral("delivery failed"));
        QCOMPARE(failed.count(), 1);
        QCOMPARE(failed.first().first().toString(), QStringLiteral("delivery failed"));
    }

    // Rebinding to a new conversation clears the prior projection and refreshes the
    // new one (a controller reused across tab reassignment must not leak rows).
    void rebindClearsPriorTranscript() {
        MockChatService svc;
        svc.seed(kT, kC, {msg(QStringLiteral("Alice"), QStringLiteral("first"))});
        svc.seed(kT, QStringLiteral("!second:demo"),
                 {msg(QStringLiteral("Carol"), QStringLiteral("second"))});
        ChatConversationController convo;
        convo.setService(&svc);

        convo.open(kT, kC);
        QVERIFY(convo.markdown().contains(QStringLiteral("first")));
        convo.open(kT, QStringLiteral("!second:demo"));
        QVERIFY(!convo.markdown().contains(QStringLiteral("first")));
        QVERIFY(convo.markdown().contains(QStringLiteral("second")));
    }

    // The static projection is pure + directly usable by both surfaces.
    void projectMarkdownIsPure() {
        const QString out = ChatConversationController::projectMarkdown(
            {msg(QStringLiteral("Alice"), QStringLiteral("one")),
             msg(QStringLiteral("Bob"), QStringLiteral("two"))});
        QVERIFY(out.contains(QStringLiteral("one")));
        QVERIFY(out.contains(QStringLiteral("two")));
        QVERIFY(out.indexOf(QStringLiteral("one")) < out.indexOf(QStringLiteral("two")));
    }
};

QTEST_MAIN(TestChatConversationController)
#include "tst_chat_conversation_controller.moc"
