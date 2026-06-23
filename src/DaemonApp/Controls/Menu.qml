import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// Themed Menu - the kit's replacement for the bland Basic-styled QQC.Menu used by
// context menus. Restyles only the frame (token-driven surface + border + radius,
// no Material shadow); the rows are themed by Kit.MenuItem / Kit.MenuSeparator
// declared as children. Use it anywhere a right-click / popup() menu is needed.
QQC.Menu {
    id: root

    padding: 6
    // Render in-scene (never a native menu window) so the QML styling always
    // applies regardless of platform/style.
    popupType: QQC.Popup.Item

    background: Rectangle {
        implicitWidth: 200
        color: Theme.background
        border.color: Theme.border
        border.width: 1
        radius: 10
    }
}
