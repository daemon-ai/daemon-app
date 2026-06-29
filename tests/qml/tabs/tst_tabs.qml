// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtTest
import QtQuick.Layouts
import DaemonApp.Tabs
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Behavioral coverage for the pane tab strip: the shared TabModel drives a real
// TabBar (chips) and a StackLayout (pages) exactly as the Session host wires
// them. Asserts add/remove/switch, find-or-create re-activation, the "+"/settings
// affordances, and that clicking a chip activates it and flips the visible page.
TestCase {
    id: tc
    name: "Tabs"
    width: 800
    height: 240
    when: windowShown

    TabModel {
        id: tabModel
    }

    TabBar {
        id: tabBar
        x: 0
        y: 0
        width: 600
        height: 40
        tabModel: tabModel
    }

    StackLayout {
        id: stack
        x: 0
        // Below the tab strip so it never intercepts clicks on the chips/buttons.
        y: 60
        width: 600
        height: 120
        // The same binding Session.qml uses to show the active tab's page.
        currentIndex: tabModel.currentIndex

        Repeater {
            model: tabModel
            delegate: Rectangle {
                objectName: "stackPage"
                required property int tabId
                color: "transparent"
            }
        }
    }

    // Mirrors the real Session.qml tab body: a plain Item with a Repeater of
    // Loaders that anchors.fill the body and toggle visible on the active index.
    // This guards the layout fix - the regression was pages with zero size, so we
    // assert each loaded page fills the body and only the active one is visible.
    Item {
        id: body
        x: 0
        y: 200
        width: 600
        height: 120

        Repeater {
            id: bodyRepeater
            model: tabModel
            delegate: Loader {
                id: pageLoader
                required property int index
                required property string sessionId
                property bool pageVisible: index === tabModel.currentIndex
                anchors.fill: parent
                visible: pageVisible
                sourceComponent: Rectangle {
                    objectName: "bodyPage"
                    property string boundSessionId: pageLoader.sessionId
                }
                onLoaded: item.boundSessionId = Qt.binding(() => pageLoader.sessionId)
            }
        }
    }

    SignalSpy {
        id: newSpy
        target: tabBar
        signalName: "newTabRequested"
    }
    SignalSpy {
        id: settingsSpy
        target: tabBar
        signalName: "settingsRequested"
    }

    function init() {
        while (tabModel.count > 0)
            tabModel.closeTab(0);
        newSpy.clear();
        settingsSpy.clear();
    }

    function tabListView() {
        return findChild(tabBar, "tabListView");
    }

    // Opening sessions appends chips, activates the newest, and the bound
    // StackLayout follows the model's active index.
    function test_open_adds_chips_and_tracks_active() {
        tabModel.openTranscript("1", "Alpha");
        tabModel.openTranscript("2", "Beta");

        const lv = tabListView();
        verify(lv !== null, "tab ListView exists");
        compare(lv.count, 2, "two chips rendered");
        compare(tabModel.currentIndex, 1, "newest tab active");
        compare(stack.currentIndex, 1, "StackLayout follows the active tab");
    }

    // Re-opening an already-open session re-activates its tab (no duplicate).
    function test_reopen_reuses_existing_tab() {
        tabModel.openTranscript("1", "Alpha");
        tabModel.openTranscript("2", "Beta");
        tabModel.openTranscript("1", "Alpha");

        compare(tabListView().count, 2, "no duplicate chip");
        compare(tabModel.currentIndex, 0, "re-activated the existing tab");
    }

    // Closing the active tab removes its chip and selects the right-hand
    // neighbour; the StackLayout stays in sync.
    function test_close_removes_and_switches() {
        tabModel.openTranscript("1", "Alpha");
        tabModel.openTranscript("2", "Beta");
        tabModel.openTranscript("3", "Gamma");
        tabModel.activate(1); // Beta

        tabModel.closeTab(1);
        compare(tabListView().count, 2, "chip removed");
        compare(stack.currentIndex, tabModel.currentIndex, "stack tracks model");
        compare(tabModel.sessionIdAt(tabModel.currentIndex), "3", "Gamma is active");
    }

    // The Settings page opens as a singleton page tab alongside transcripts.
    function test_settings_page_tab() {
        tabModel.openTranscript("1", "Alpha");
        tabModel.openPage(TabModel.Settings, "Settings");
        compare(tabListView().count, 2, "transcript + settings");
        compare(tabModel.kindAt(tabModel.currentIndex), TabModel.Settings, "settings active");

        tabModel.openPage(TabModel.Settings, "Settings"); // singleton
        compare(tabListView().count, 2, "settings is a singleton");
    }

    // The "+" affordance requests a new tab; the gear requests settings. The
    // buttons' clicked() is wired to the TabBar's signals (emitting it directly
    // exercises that wiring deterministically offscreen).
    function test_affordances_emit() {
        findChild(tabBar, "newTabButton").clicked();
        compare(newSpy.count, 1, "new tab requested");

        findChild(tabBar, "settingsButton").clicked();
        compare(settingsSpy.count, 1, "settings requested");
    }

    // Clicking a background chip activates it and flips the visible page.
    function test_click_chip_activates() {
        tabModel.openTranscript("1", "Alpha");
        tabModel.openTranscript("2", "Beta"); // active = index 1
        compare(tabModel.currentIndex, 1);

        const lv = tabListView();
        const chip = lv.itemAtIndex(0);
        verify(chip !== null, "first chip realized");
        // Activate via the chip's click handler (synthetic mouse delivery to a
        // Flickable delegate is unreliable offscreen; this exercises the same
        // MouseArea.onClicked -> TabModel.activate wiring the GUI uses).
        findChild(chip, "tabChipArea").clicked(null);

        compare(tabModel.currentIndex, 0, "clicked chip became active");
        compare(stack.currentIndex, 0, "stack switched to the clicked page");
    }

    // A preview open reuses the single preview chip in place (VSCode-style).
    function test_preview_reuses_single_chip() {
        tabModel.previewTranscript("1", "Alpha");
        verify(tabModel.isPreviewAt(0), "first preview is a preview tab");

        tabModel.previewTranscript("2", "Beta");
        compare(tabListView().count, 1, "preview reused, not appended");
        compare(tabModel.sessionIdAt(0), "2", "preview reassigned to Beta");
        verify(tabModel.isPreviewAt(0), "still a preview tab");
    }

    // Double-clicking a preview chip pins it (makes it permanent), so the next
    // preview opens a fresh chip instead of replacing it.
    function test_double_click_pins_chip() {
        tabModel.previewTranscript("1", "Alpha");
        const chip = tabListView().itemAtIndex(0);
        verify(chip !== null, "preview chip realized");
        findChild(chip, "tabChipArea").doubleClicked(null);
        verify(!tabModel.isPreviewAt(0), "double-click pinned the chip");

        tabModel.previewTranscript("2", "Beta");
        compare(tabListView().count, 2, "next preview opened a fresh chip");
    }

    // A deliberate open (openTranscript) of the previewed session pins it.
    function test_open_pins_preview() {
        tabModel.previewTranscript("1", "Alpha");
        verify(tabModel.isPreviewAt(0), "preview before open");
        tabModel.openTranscript("1", "Alpha");
        verify(!tabModel.isPreviewAt(0), "open pinned the preview");
        compare(tabListView().count, 1, "no duplicate chip");
    }

    function bodyPages() {
        // Collect the loaded page Items via the Repeater (findChildren is not
        // available in QtTest's TestCase).
        const pages = [];
        for (let i = 0; i < bodyRepeater.count; ++i) {
            const loader = bodyRepeater.itemAt(i);
            if (loader && loader.item)
                pages.push(loader.item);
        }
        return pages;
    }

    // The layout fix: each loaded page fills the body (non-zero, parent-sized) and
    // only the active tab's page is visible. This is exactly what regressed before
    // (StackLayout + Loader with no fill -> zero-size, invisible transcript).
    function test_body_pages_fill_and_toggle_visibility() {
        tabModel.openTranscript("1", "Alpha");
        tabModel.openTranscript("2", "Beta"); // active = index 1
        wait(0); // let the Loaders instantiate

        const pages = bodyPages();
        compare(pages.length, 2, "one page per tab");
        for (let i = 0; i < pages.length; ++i) {
            compare(pages[i].width, body.width, "page fills body width");
            compare(pages[i].height, body.height, "page fills body height");
            verify(pages[i].width > 0 && pages[i].height > 0, "page has non-zero size");
        }

        // Visibility follows the active index (the Loader toggles `visible`).
        compare(tabModel.currentIndex, 1, "Beta active");
        compare(bodyRepeater.itemAt(0).index, 0, "loader 0 has index 0");
        compare(bodyRepeater.itemAt(1).index, 1, "loader 1 has index 1");
        compare(bodyRepeater.itemAt(1).pageVisible, true, "active page visible");
        compare(bodyRepeater.itemAt(0).pageVisible, false, "background page hidden");

        tabModel.activate(0);
        compare(bodyRepeater.itemAt(0).pageVisible, true, "newly active page visible");
        compare(bodyRepeater.itemAt(1).pageVisible, false, "previously active page hidden");
    }

    // A reassigned preview tab reloads in place: the page's bound sessionId
    // updates (this is what makes TranscriptPage.onSessionIdChanged reload).
    function test_preview_reassign_rebinds_page() {
        tabModel.previewTranscript("7", "Alpha");
        wait(0);
        let pages = bodyPages();
        compare(pages.length, 1, "single preview page");
        compare(pages[0].boundSessionId, "7", "page bound to session 7");

        tabModel.previewTranscript("8", "Beta"); // reuse the same slot
        wait(0);
        pages = bodyPages();
        compare(pages.length, 1, "still one page (reused)");
        compare(pages[0].boundSessionId, "8", "page rebound to session 8");
    }
}
