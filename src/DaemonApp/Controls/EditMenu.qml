import QtQuick

// Themed editing context menu for text inputs - the kit's replacement for the
// default Basic-styled context menu Qt 6.9+ attaches to TextField/TextArea. Bind
// `target` to the text control and popup() it from a right-click TapHandler (the
// control should set ContextMenu.menu: null to suppress the built-in one). Cut/
// Copy/Paste enable from the target's selection/clipboard state.
Menu {
    id: root

    // The TextField / TextArea this menu acts on.
    property var target: null

    MenuItem {
        text: qsTr("Cut")
        enabled: root.target && !root.target.readOnly
                 && root.target.selectedText.length > 0
        onTriggered: if (root.target) root.target.cut()
    }
    MenuItem {
        text: qsTr("Copy")
        enabled: root.target && root.target.selectedText.length > 0
        onTriggered: if (root.target) root.target.copy()
    }
    MenuItem {
        text: qsTr("Paste")
        enabled: root.target && !root.target.readOnly && root.target.canPaste
        onTriggered: if (root.target) root.target.paste()
    }
    MenuSeparator {}
    MenuItem {
        text: qsTr("Select All")
        enabled: root.target && root.target.length > 0
        onTriggered: if (root.target) root.target.selectAll()
    }
}
