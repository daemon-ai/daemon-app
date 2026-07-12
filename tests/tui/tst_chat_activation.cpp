// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [integrations wire v38] Work package A4 (app/chat-tabs): the activation-dispatch
// rewire. Activating a room/DM conversation must open (or focus) a native Chat tab
// for (transport, conversation) — NOT the Channels admin page it fell back to
// before. Both surfaces route the sidebar's conversationActivated(transport, conv)
// through rwdetail::openConversationTab (GUI Main.qml + TUI RootWidget), so driving
// that shared helper against the real shared TabModel guards the rewire. Non-chat
// activations (sessions/pages) keep their existing paths (covered elsewhere).

#include "root_widget_detail.h"
#include "tab_model.h"

#include <QSignalSpy>
#include <QtTest>

class TestChatActivation : public QObject {
    Q_OBJECT

private slots:
    // Activating a conversation opens a Chat tab keyed by (transport, conv) and
    // activates it — never a Channels page.
    void conversationActivationOpensChatTab() {
        TabModel tabs;
        QSignalSpy currentSpy(&tabs, &TabModel::currentTabChanged);

        const int id =
            rwdetail::openConversationTab(&tabs, QStringLiteral("demo/acct"),
                                          QStringLiteral("!room:demo"), QStringLiteral("#general"));
        QVERIFY(id > 0);
        QCOMPARE(tabs.count(), 1);
        QCOMPARE(tabs.kindAt(0), static_cast<int>(TabModel::Chat));
        QCOMPARE(tabs.transportAt(0), QStringLiteral("demo/acct"));
        QCOMPARE(tabs.conversationAt(0), QStringLiteral("!room:demo"));
        QCOMPARE(tabs.titleAt(0), QStringLiteral("#general"));
        QCOMPARE(tabs.currentIndex(), 0);
        QCOMPARE(currentSpy.count(), 1);
        // The rewire's whole point: NO Channels page was opened.
        for (int i = 0; i < tabs.count(); ++i) {
            QVERIFY(tabs.kindAt(i) != static_cast<int>(TabModel::Channels));
        }
    }

    // Re-activating the same conversation focuses its existing tab (find-or-create).
    void reactivatingConversationFocusesExistingTab() {
        TabModel tabs;
        const int first = rwdetail::openConversationTab(&tabs, QStringLiteral("demo/acct"),
                                                        QStringLiteral("!room:demo"), QString());
        rwdetail::openConversationTab(&tabs, QStringLiteral("demo/acct"),
                                      QStringLiteral("@bob:demo"), QStringLiteral("Bob"));
        QCOMPARE(tabs.count(), 2);

        const int again = rwdetail::openConversationTab(&tabs, QStringLiteral("demo/acct"),
                                                        QStringLiteral("!room:demo"), QString());
        QCOMPARE(again, first);
        QCOMPARE(tabs.count(), 2); // reused, not duplicated
        QCOMPARE(tabs.currentIndex(), 0);
    }

    // An empty label falls back to the conversation id (never a blank chip).
    void emptyLabelFallsBackToConversationId() {
        TabModel tabs;
        rwdetail::openConversationTab(&tabs, QStringLiteral("demo/acct"),
                                      QStringLiteral("!room:demo"), QString());
        QCOMPARE(tabs.titleAt(0), QStringLiteral("!room:demo"));
    }
};

QTEST_MAIN(TestChatActivation)
#include "tst_chat_activation.moc"
