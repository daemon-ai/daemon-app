import QtQuick

// Empty top-level window (QQuickWindow). The app shell - sidebar, conversations
// list, and the conversation (transcript + composer) - is composed here as the
// feature modules come online.
Window {
    id: root

    width: 1024
    height: 720
    visible: true
    title: qsTr("daemon-app")
}
