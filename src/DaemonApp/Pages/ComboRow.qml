import QtQuick
import DaemonApp.Controls as Kit

// A labelled settings row whose control is a dropdown bound to a DaemonConfig
// key. `options` are the choice strings; the current value is read from
// DaemonConfig and written back on selection.
SettingRow {
    id: root
    property string configKey: ""
    property var options: []

    Kit.Dropdown {
        implicitWidth: 170
        model: root.options
        Component.onCompleted: {
            const cur = DaemonConfig.value(root.configKey);
            const idx = root.options.indexOf(cur);
            currentIndex = idx >= 0 ? idx : 0;
        }
        onActivated: DaemonConfig.setValue(root.configKey, root.options[currentIndex])
    }
}
