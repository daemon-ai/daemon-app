import QtQuick
import QtTest
import QtQuick.Layouts
import DaemonApp.Tabs
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Behavioral coverage for the pane tab strip: the shared TabModel drives a real
// TabBar (chips) and a StackLayout (pages) exactly as the Conversation host wires
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
        // The same binding Conversation.qml uses to show the active tab's page.
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

    // Opening conversations appends chips, activates the newest, and the bound
    // StackLayout follows the model's active index.
    function test_open_adds_chips_and_tracks_active() {
        tabModel.openTranscript(1, "Alpha");
        tabModel.openTranscript(2, "Beta");

        const lv = tabListView();
        verify(lv !== null, "tab ListView exists");
        compare(lv.count, 2, "two chips rendered");
        compare(tabModel.currentIndex, 1, "newest tab active");
        compare(stack.currentIndex, 1, "StackLayout follows the active tab");
    }

    // Re-opening an already-open conversation re-activates its tab (no duplicate).
    function test_reopen_reuses_existing_tab() {
        tabModel.openTranscript(1, "Alpha");
        tabModel.openTranscript(2, "Beta");
        tabModel.openTranscript(1, "Alpha");

        compare(tabListView().count, 2, "no duplicate chip");
        compare(tabModel.currentIndex, 0, "re-activated the existing tab");
    }

    // Closing the active tab removes its chip and selects the right-hand
    // neighbour; the StackLayout stays in sync.
    function test_close_removes_and_switches() {
        tabModel.openTranscript(1, "Alpha");
        tabModel.openTranscript(2, "Beta");
        tabModel.openTranscript(3, "Gamma");
        tabModel.activate(1); // Beta

        tabModel.closeTab(1);
        compare(tabListView().count, 2, "chip removed");
        compare(stack.currentIndex, tabModel.currentIndex, "stack tracks model");
        compare(tabModel.conversationIdAt(tabModel.currentIndex), 3, "Gamma is active");
    }

    // The Settings page opens as a singleton page tab alongside transcripts.
    function test_settings_page_tab() {
        tabModel.openTranscript(1, "Alpha");
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
        tabModel.openTranscript(1, "Alpha");
        tabModel.openTranscript(2, "Beta"); // active = index 1
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
}
