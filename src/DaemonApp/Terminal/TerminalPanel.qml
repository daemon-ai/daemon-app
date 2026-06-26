import QtQuick
import QtQuick.Layouts
import QMLTermWidget 2.0
import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.Controls as Kit

// An embedded shell terminal, docked at the bottom of the session pane (the
// VSCode-style split). A slim header (title + close) sits over a QMLTermWidget
// running an interactive bash session, rooted at the last-used directory (or
// $HOME on first run). The PTY is created lazily the first time the panel becomes
// visible and kept alive afterwards, so toggling the panel off/on does not kill
// the running shell. Colors follow the active app theme via the baked
// "daemon-<Theme>" color schemes shipped with the QMLTermWidget plugin.
Rectangle {
    id: root

    color: Theme.background

    // Set by the host when the toggle is flipped off, so the close button and the
    // status-bar button share one source of truth.
    signal closeRequested()

    // Start the shell the first time we are shown so we do not spawn a bash
    // process for users who never open the terminal. This must also fire from
    // Component.onCompleted: when showTerminal is persisted on, the panel is
    // created already visible, so onVisibleChanged never fires (the value never
    // changes from the Item default `true`) and the shell would never start -
    // leaving a black surface on reopen.
    property bool _started: false
    function _ensureStarted() {
        if (!root._started) {
            root._started = true;
            // Set the working dir imperatively right before launch: a declarative
            // binding is not guaranteed to be applied to the session before this
            // runs, and the session reads it only once at startShellProgram().
            const dir = UiSettings.terminalWorkingDirectory;
            ptySession.initialWorkingDirectory = (dir && dir.length > 0) ? dir : "$HOME";
            ptySession.startShellProgram();
        }
    }
    // QMLTermWidget only receives key events when it holds Qt active focus, and
    // it does not self-focus on click. Focus it whenever the panel appears -
    // deferred so it applies after the window is active on reopen (where
    // onVisibleChanged never fires because the panel loads already visible).
    function _focusTerminal() {
        Qt.callLater(function() { terminal.forceActiveFocus(); });
    }
    Component.onCompleted: if (visible) { root._ensureStarted(); root._focusTerminal(); }
    onVisibleChanged: {
        if (visible) {
            root._ensureStarted();
            root._focusTerminal();
        }
    }

    // Persist the shell's working directory so a reopened terminal resumes where
    // it left off. currentDir has no change signal, so poll it while the shell is
    // running and save on changes; also capture it once on teardown (clean quit).
    function _saveCwd() {
        const dir = ptySession.currentDir;
        if (dir && dir.length > 0)
            UiSettings.terminalWorkingDirectory = dir;
    }
    Timer {
        interval: 4000
        repeat: true
        running: root._started
        onTriggered: root._saveCwd()
    }
    Component.onDestruction: if (root._started) root._saveCwd()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Terminal surface ----------------------------------------------
        QMLTermWidget {
            id: terminal
            Layout.fillWidth: true
            Layout.fillHeight: true

            font.family: FontIcons.mono
            font.pointSize: 11 + Theme.pointSizeOffset
            // Follow the app theme: the plugin ships daemon-Light/Dark/Sepia/
            // Midnight schemes matching our palette, recoloring live on switch.
            colorScheme: "daemon-" + Theme.theme

            session: QMLTermSession {
                id: ptySession
                // initialWorkingDirectory is set imperatively in _ensureStarted()
                // (the session reads it once at startShellProgram). Launch bash via
                // env so it resolves on PATH (robust on NixOS, no /bin/bash).
                shellProgram: "/usr/bin/env"
                shellProgramArgs: ["bash"]
            }

            // The widget never self-focuses on click, so clicking back into the
            // terminal (after focus moved to the composer, etc.) would not restore
            // typing. Grab focus on press, then decline the event so the widget
            // still gets it for text selection/drag.
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: function(mouse) {
                    terminal.forceActiveFocus();
                    mouse.accepted = false;
                }
            }

            // Our own auto-hiding scrollbar overlay. Replaces the plugin's bundled
            // QMLTermScrollbar (which trips a deprecated-Connections warning) and
            // lets us theme the thumb. Reads the same scrollbar* properties.
            Item {
                id: scrollbar
                anchors.right: parent.right
                width: 8

                readonly property int value: terminal.scrollbarCurrentValue
                readonly property int minimum: terminal.scrollbarMinimum
                readonly property int maximum: terminal.scrollbarMaximum
                readonly property int lines: terminal.lines
                readonly property int totalLines: lines + maximum

                opacity: 0.0
                height: totalLines > minimum
                      ? terminal.height * (lines / (totalLines - minimum)) : 0
                y: totalLines > 0
                 ? (terminal.height / totalLines) * (value - minimum) : 0

                Behavior on opacity {
                    NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: width * 0.5
                    color: Theme.iconMuted
                    opacity: 0.5
                }

                Connections {
                    target: terminal
                    function onScrollbarValueChanged() {
                        scrollbar.opacity = 1.0;
                        hideTimer.restart();
                    }
                }

                Timer {
                    id: hideTimer
                    interval: 1000
                    onTriggered: scrollbar.opacity = 0.0
                }
            }
        }
    }
}
