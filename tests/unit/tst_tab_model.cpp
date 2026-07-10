// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "tab_model.h"

#include <QSignalSpy>
#include <QtTest>

// Exercises the shared pane-tab model: find-or-create open semantics, active-tab
// tracking via currentIndex/CurrentRole, close-neighbour selection, the Settings
// singleton page, cycling, and reorder. The model is the single source of truth
// both the QML GUI and the Tui Widgets TUI bind, so this guards the contract.
class TestTabModel : public QObject {
    Q_OBJECT

private:
    template <typename T>
    static T roleAt(const TabModel& m, int row, TabModel::Role role) {
        return m.data(m.index(row, 0), role).value<T>();
    }
    // Transcript session ids are opaque strings (roadmap P3); the tests use "s-N".
    static QString sid(int n) { return QStringLiteral("s-%1").arg(n); }

private slots:
    // Opening a fresh session appends a transcript tab, activates it, and
    // assigns a stable monotonic id.
    void openTranscriptAppendsAndActivates() {
        TabModel model;
        QSignalSpy currentSpy(&model, &TabModel::currentTabChanged);

        const int id0 = model.openTranscript(sid(10), QStringLiteral("Alpha"));
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.currentIndex(), 0);
        QCOMPARE(roleAt<QString>(model, 0, TabModel::SessionIdRole), sid(10));
        QCOMPARE(roleAt<QString>(model, 0, TabModel::TitleRole), QStringLiteral("Alpha"));
        QVERIFY(roleAt<bool>(model, 0, TabModel::CurrentRole));

        const int id1 = model.openTranscript(sid(20), QStringLiteral("Beta"));
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.currentIndex(), 1);
        QVERIFY(id1 != id0);
        QVERIFY(roleAt<bool>(model, 1, TabModel::CurrentRole));
        QVERIFY(!roleAt<bool>(model, 0, TabModel::CurrentRole));

        QCOMPARE(currentSpy.count(), 2);
        QCOMPARE(currentSpy.takeFirst().at(0).toInt(), id0);
        QCOMPARE(currentSpy.takeFirst().at(0).toInt(), id1);
    }

    // Re-opening an already-open session re-activates its existing tab
    // instead of creating a duplicate (and refreshes its title).
    void openTranscriptReusesExistingTab() {
        TabModel model;
        const int id0 = model.openTranscript(sid(10), QStringLiteral("Alpha"));
        model.openTranscript(sid(20), QStringLiteral("Beta")); // active -> row 1

        QSignalSpy insertSpy(&model, &TabModel::rowsInserted);
        const int again = model.openTranscript(sid(10), QStringLiteral("Alpha (edited)"));

        QCOMPARE(again, id0);
        QCOMPARE(insertSpy.count(), 0); // no new row
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.currentIndex(), 0); // re-activated the existing tab
        QCOMPARE(roleAt<QString>(model, 0, TabModel::TitleRole), QStringLiteral("Alpha (edited)"));
    }

    // The Settings page is a singleton: opening it twice activates the same tab.
    void openPageIsSingletonPerKind() {
        TabModel model;
        model.openTranscript(sid(10), QStringLiteral("Alpha"));
        const int settings = model.openPage(TabModel::Settings, QStringLiteral("Settings"));
        QCOMPARE(model.count(), 2);
        QCOMPARE(roleAt<int>(model, 1, TabModel::KindRole), int(TabModel::Settings));
        QCOMPARE(roleAt<QString>(model, 1, TabModel::SessionIdRole), QString());

        const int again = model.openPage(TabModel::Settings, QStringLiteral("Settings"));
        QCOMPARE(again, settings);
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.currentIndex(), 1);
    }

    // Closing a tab before the active one shifts the active index left but keeps
    // the same active tab id.
    void closingTabBeforeActiveShiftsIndex() {
        TabModel model;
        model.openTranscript(sid(10), QStringLiteral("Alpha"));
        const int idB = model.openTranscript(sid(20), QStringLiteral("Beta"));
        model.openTranscript(sid(30), QStringLiteral("Gamma")); // active = row 2 (Gamma)
        model.activate(1);                                      // active = Beta (row 1)
        QCOMPARE(model.currentIndex(), 1);

        model.closeTab(0); // remove Alpha
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.currentIndex(), 0); // Beta slid into row 0
        QCOMPARE(model.tabIdAt(0), idB);
        QVERIFY(roleAt<bool>(model, 0, TabModel::CurrentRole));
    }

    // Closing the active tab selects the right-hand neighbour (the slot is
    // reused); emits tabClosed with the removed id.
    void closingActiveTabSelectsNeighbour() {
        TabModel model;
        model.openTranscript(sid(10), QStringLiteral("Alpha"));
        const int idB = model.openTranscript(sid(20), QStringLiteral("Beta"));
        const int idG = model.openTranscript(sid(30), QStringLiteral("Gamma"));
        model.activate(1); // active = Beta

        QSignalSpy closedSpy(&model, &TabModel::tabClosed);
        model.closeTab(1); // remove Beta

        QCOMPARE(closedSpy.count(), 1);
        QCOMPARE(closedSpy.takeFirst().at(0).toInt(), idB);
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.currentIndex(), 1); // Gamma slid into the vacated slot
        QCOMPARE(model.tabIdAt(1), idG);
    }

    // Closing the last (active, right-most) tab falls back to the new last row.
    void closingLastActiveTabFallsBackLeft() {
        TabModel model;
        const int idA = model.openTranscript(sid(10), QStringLiteral("Alpha"));
        model.openTranscript(sid(20), QStringLiteral("Beta")); // active = row 1 (Beta)

        model.closeTab(1); // remove the active right-most tab
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.currentIndex(), 0);
        QCOMPARE(model.tabIdAt(0), idA);
    }

    // Closing the only tab empties the model and clears the active index.
    void closingOnlyTabEmptiesModel() {
        TabModel model;
        model.openTranscript(sid(10), QStringLiteral("Alpha"));

        QSignalSpy currentSpy(&model, &TabModel::currentTabChanged);
        model.closeTab(0);

        QCOMPARE(model.count(), 0);
        QCOMPARE(model.currentIndex(), -1);
        QCOMPARE(currentSpy.count(), 1);
        QCOMPARE(currentSpy.takeFirst().at(0).toInt(), -1);
    }

    // cycle() wraps around the active index in both directions.
    void cycleWrapsAround() {
        TabModel model;
        model.openTranscript(sid(10), QStringLiteral("Alpha"));
        model.openTranscript(sid(20), QStringLiteral("Beta"));
        model.openTranscript(sid(30), QStringLiteral("Gamma")); // active = 2

        model.cycle(1); // wraps 2 -> 0
        QCOMPARE(model.currentIndex(), 0);
        model.cycle(-1); // wraps 0 -> 2
        QCOMPARE(model.currentIndex(), 2);
        model.cycle(-1); // 2 -> 1
        QCOMPARE(model.currentIndex(), 1);
    }

    // Reordering keeps the active tab pointing at the same tab identity.
    void moveTabKeepsActiveIdentity() {
        TabModel model;
        const int idA = model.openTranscript(sid(10), QStringLiteral("Alpha"));
        model.openTranscript(sid(20), QStringLiteral("Beta"));
        model.openTranscript(sid(30), QStringLiteral("Gamma"));
        model.activate(0); // active = Alpha (row 0)

        model.moveTab(0, 2); // Alpha goes to the end
        QCOMPARE(model.tabIdAt(2), idA);
        QCOMPARE(model.currentIndex(), 2); // followed Alpha
        QVERIFY(roleAt<bool>(model, 2, TabModel::CurrentRole));
    }

    // setCurrentIndex (the QML-writable property) activates by row and is a no-op
    // out of range.
    void setCurrentIndexActivates() {
        TabModel model;
        model.openTranscript(sid(10), QStringLiteral("Alpha"));
        model.openTranscript(sid(20), QStringLiteral("Beta"));

        model.setCurrentIndex(0);
        QCOMPARE(model.currentIndex(), 0);
        model.setCurrentIndex(99); // out of range -> ignored
        QCOMPARE(model.currentIndex(), 0);
    }

    // previewTranscript opens a transient tab; previewing a different session
    // reuses that single slot (reassigns it in place) rather than appending.
    void previewReusesSingleSlot() {
        TabModel model;
        QSignalSpy reassignSpy(&model, &TabModel::tabSessionChanged);

        const int id0 = model.previewTranscript(sid(10), QStringLiteral("Alpha"));
        QCOMPARE(model.count(), 1);
        QVERIFY(roleAt<bool>(model, 0, TabModel::PreviewRole));

        const int id1 = model.previewTranscript(sid(20), QStringLiteral("Beta"));
        QCOMPARE(id1, id0);         // same (stable) tab id
        QCOMPARE(model.count(), 1); // reused, not appended
        QCOMPARE(roleAt<QString>(model, 0, TabModel::SessionIdRole), sid(20));
        QCOMPARE(roleAt<QString>(model, 0, TabModel::TitleRole), QStringLiteral("Beta"));
        QVERIFY(roleAt<bool>(model, 0, TabModel::PreviewRole)); // still preview

        QCOMPARE(reassignSpy.count(), 1);
        QCOMPARE(reassignSpy.takeFirst().at(0).toInt(), id0);
    }

    // At most one preview tab ever exists, regardless of how many previews open.
    void previewSingleInvariant() {
        TabModel model;
        model.previewTranscript(sid(10), QStringLiteral("Alpha"));
        model.previewTranscript(sid(20), QStringLiteral("Beta"));
        model.previewTranscript(sid(30), QStringLiteral("Gamma"));
        QCOMPARE(model.count(), 1);
        QCOMPARE(roleAt<QString>(model, 0, TabModel::SessionIdRole), sid(30));
    }

    // Previewing a session that is already open in some tab activates that
    // tab instead of reassigning the preview slot.
    void previewActivatesExistingTab() {
        TabModel model;
        const int idA = model.openTranscript(sid(10), QStringLiteral("Alpha"));   // pinned, row 0
        const int idP = model.previewTranscript(sid(20), QStringLiteral("Beta")); // preview, row 1
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.currentIndex(), 1);

        const int again = model.previewTranscript(sid(10), QStringLiteral("Alpha"));
        QCOMPARE(again, idA);
        QCOMPARE(model.count(), 2);        // nothing reassigned/appended
        QCOMPARE(model.currentIndex(), 0); // activated the existing pinned tab
        // The preview slot is untouched (still Beta).
        QCOMPARE(model.tabIdAt(1), idP);
        QCOMPARE(roleAt<QString>(model, 1, TabModel::SessionIdRole), sid(20));
    }

    // Pinning the preview tab makes it permanent: the next preview opens a fresh
    // slot rather than replacing it.
    void pinPromotesPreviewSoNextPreviewAppends() {
        TabModel model;
        model.previewTranscript(sid(10), QStringLiteral("Alpha"));
        QVERIFY(roleAt<bool>(model, 0, TabModel::PreviewRole));

        model.pinTab(0);
        QVERIFY(!roleAt<bool>(model, 0, TabModel::PreviewRole));

        model.previewTranscript(sid(20), QStringLiteral("Beta"));
        QCOMPARE(model.count(), 2); // appended, the pinned tab was not reused
        QCOMPARE(roleAt<QString>(model, 0, TabModel::SessionIdRole), sid(10));
        QCOMPARE(roleAt<QString>(model, 1, TabModel::SessionIdRole), sid(20));
        QVERIFY(roleAt<bool>(model, 1, TabModel::PreviewRole));
    }

    // A deliberate open (openTranscript) of the previewed session pins it.
    void openTranscriptPinsThePreview() {
        TabModel model;
        const int idP = model.previewTranscript(sid(10), QStringLiteral("Alpha"));
        QVERIFY(roleAt<bool>(model, 0, TabModel::PreviewRole));

        const int opened = model.openTranscript(sid(10), QStringLiteral("Alpha"));
        QCOMPARE(opened, idP);
        QCOMPARE(model.count(), 1);
        QVERIFY(!roleAt<bool>(model, 0, TabModel::PreviewRole));
    }

    void previewFileReusesTranscriptPreviewAndSignalsKindChange() {
        TabModel model;
        const int id = model.previewTranscript(sid(10), QStringLiteral("Alpha"));
        QSignalSpy kindSpy(&model, &TabModel::tabKindChanged);
        QSignalSpy currentSpy(&model, &TabModel::currentTabChanged);

        const int fileId =
            model.previewFile(QStringLiteral("workspace"), QStringLiteral("src/main.cpp"),
                              QStringLiteral("main.cpp"));

        QCOMPARE(fileId, id);
        QCOMPARE(model.count(), 1);
        QCOMPARE(roleAt<int>(model, 0, TabModel::KindRole), int(TabModel::File));
        QCOMPARE(roleAt<QString>(model, 0, TabModel::SessionIdRole), QString());
        QCOMPARE(roleAt<QString>(model, 0, TabModel::FileRootRole), QStringLiteral("workspace"));
        QCOMPARE(roleAt<QString>(model, 0, TabModel::FilePathRole), QStringLiteral("src/main.cpp"));
        QCOMPARE(kindSpy.count(), 1);
        QCOMPARE(kindSpy.takeFirst().at(0).toInt(), id);
        QVERIFY(currentSpy.count() >= 1); // already-active preview still rebinds
    }

    void previewTranscriptReusesFilePreviewAndSignalsKindChange() {
        TabModel model;
        const int id = model.previewFile(QStringLiteral("workspace"), QStringLiteral("README.md"),
                                         QStringLiteral("README.md"));
        QSignalSpy kindSpy(&model, &TabModel::tabKindChanged);

        const int transcriptId = model.previewTranscript(sid(42), QStringLiteral("Answer"));

        QCOMPARE(transcriptId, id);
        QCOMPARE(model.count(), 1);
        QCOMPARE(roleAt<int>(model, 0, TabModel::KindRole), int(TabModel::Transcript));
        QCOMPARE(roleAt<QString>(model, 0, TabModel::SessionIdRole), sid(42));
        QCOMPARE(roleAt<QString>(model, 0, TabModel::FileRootRole), QString());
        QCOMPARE(roleAt<QString>(model, 0, TabModel::FilePathRole), QString());
        QCOMPARE(kindSpy.count(), 1);
    }

    void dirtyFilePreviewIsNotReused() {
        TabModel model;
        const int dirtyId = model.previewFile(QStringLiteral("workspace"), QStringLiteral("a.cpp"),
                                              QStringLiteral("a.cpp"));
        model.setDirtyById(dirtyId, true);

        const int nextId = model.previewFile(QStringLiteral("workspace"), QStringLiteral("b.cpp"),
                                             QStringLiteral("b.cpp"));

        QVERIFY(nextId != dirtyId);
        QCOMPARE(model.count(), 2);
        QCOMPARE(roleAt<QString>(model, 0, TabModel::FilePathRole), QStringLiteral("a.cpp"));
        QCOMPARE(roleAt<bool>(model, 0, TabModel::DirtyRole), true);
        QCOMPARE(roleAt<QString>(model, 1, TabModel::FilePathRole), QStringLiteral("b.cpp"));
    }

    // [integrations wire v38] Chat tabs (A4): activating a room/DM opens a Chat
    // kind tab keyed by (transport, conversation), carrying both ids + an empty
    // sessionId, and activates it.
    void openConversationAppendsActivatesAndCarriesKeys() {
        TabModel model;
        QSignalSpy currentSpy(&model, &TabModel::currentTabChanged);

        const int id = model.openConversation(
            QStringLiteral("demo/acct"), QStringLiteral("!room:demo"), QStringLiteral("#general"));
        QVERIFY(id > 0);
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.currentIndex(), 0);
        QCOMPARE(roleAt<int>(model, 0, TabModel::KindRole), int(TabModel::Chat));
        QCOMPARE(roleAt<QString>(model, 0, TabModel::TransportRole), QStringLiteral("demo/acct"));
        QCOMPARE(roleAt<QString>(model, 0, TabModel::ConversationRole),
                 QStringLiteral("!room:demo"));
        QCOMPARE(roleAt<QString>(model, 0, TabModel::TitleRole), QStringLiteral("#general"));
        QCOMPARE(roleAt<QString>(model, 0, TabModel::SessionIdRole), QString());
        QVERIFY(roleAt<bool>(model, 0, TabModel::CurrentRole));
        QCOMPARE(model.transportAt(0), QStringLiteral("demo/acct"));
        QCOMPARE(model.conversationAt(0), QStringLiteral("!room:demo"));
        QCOMPARE(currentSpy.count(), 1);
    }

    // Find-or-create per (transport, conversation): re-activating the same
    // conversation reuses its tab (and refreshes the title); a different
    // conversation, or the same conversation on a different transport, appends.
    void openConversationDedupesPerTransportConv() {
        TabModel model;
        const int room = model.openConversation(
            QStringLiteral("demo/acct"), QStringLiteral("!room:demo"), QStringLiteral("#room"));
        const int dm = model.openConversation(QStringLiteral("demo/acct"),
                                              QStringLiteral("@bob:demo"), QStringLiteral("Bob"));
        QCOMPARE(model.count(), 2);
        QVERIFY(dm != room);

        QSignalSpy insertSpy(&model, &TabModel::rowsInserted);
        const int again =
            model.openConversation(QStringLiteral("demo/acct"), QStringLiteral("!room:demo"),
                                   QStringLiteral("#room (topic)"));
        QCOMPARE(again, room);          // reused
        QCOMPARE(insertSpy.count(), 0); // no new row
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.currentIndex(), 0); // re-activated the room tab
        QCOMPARE(roleAt<QString>(model, 0, TabModel::TitleRole), QStringLiteral("#room (topic)"));

        // Same conversation id on a different transport is a distinct tab.
        const int otherAccount = model.openConversation(
            QStringLiteral("demo/acct2"), QStringLiteral("!room:demo"), QStringLiteral("#room@2"));
        QVERIFY(otherAccount != room);
        QCOMPARE(model.count(), 3);
    }

    // A chat tab is not a page singleton: two different conversations coexist, and
    // closing one keeps the other (restore/close parity with the other tab kinds).
    void chatTabsCoexistAndCloseIndependently() {
        TabModel model;
        const int a = model.openConversation(QStringLiteral("demo/acct"), QStringLiteral("!a:demo"),
                                             QStringLiteral("A"));
        const int b = model.openConversation(QStringLiteral("demo/acct"), QStringLiteral("!b:demo"),
                                             QStringLiteral("B"));
        QCOMPARE(model.count(), 2);

        QSignalSpy closedSpy(&model, &TabModel::tabClosed);
        model.closeTabById(a);
        QCOMPARE(closedSpy.count(), 1);
        QCOMPARE(closedSpy.takeFirst().at(0).toInt(), a);
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.tabIdAt(0), b);
        QCOMPARE(roleAt<QString>(model, 0, TabModel::ConversationRole), QStringLiteral("!b:demo"));
    }

    void previewFileReassignmentEmitsFileChanged() {
        TabModel model;
        const int id = model.previewFile(QStringLiteral("workspace"), QStringLiteral("a.cpp"),
                                         QStringLiteral("a.cpp"));
        QSignalSpy fileSpy(&model, &TabModel::tabFileChanged);

        const int again = model.previewFile(QStringLiteral("workspace"), QStringLiteral("b.cpp"),
                                            QStringLiteral("b.cpp"));

        QCOMPARE(again, id);
        QCOMPARE(model.count(), 1);
        QCOMPARE(roleAt<QString>(model, 0, TabModel::FilePathRole), QStringLiteral("b.cpp"));
        QCOMPARE(fileSpy.count(), 1);
        const QList<QVariant> args = fileSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), id);
        QCOMPARE(args.at(1).toString(), QStringLiteral("workspace"));
        QCOMPARE(args.at(2).toString(), QStringLiteral("b.cpp"));
    }
};

QTEST_MAIN(TestTabModel)
#include "tst_tab_model.moc"
