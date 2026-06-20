import QtQuick

// Local completion data source for the composer's slash / @ triggers. There is
// no agent gateway wired, so the data is static: a small set of client-side
// slash commands (some bound to real app actions) and canned @ mentions.
//
// search(kind, query) -> array of { label, hint, group, value, action }.
//   action: "insert" inserts `value` as text; any other string is a command the
//   composer forwards to the host via commandInvoked(action).
QtObject {
    id: root

    readonly property var slashCommands: [
        { label: "/new", hint: qsTr("Start a new conversation"), group: "Command", value: "/new", action: "new" },
        { label: "/theme", hint: qsTr("Open theme & settings"), group: "Command", value: "/theme", action: "theme" },
        { label: "/distraction", hint: qsTr("Toggle distraction-free"), group: "Command", value: "/distraction", action: "distraction" },
        { label: "/clear", hint: qsTr("Clear the composer"), group: "Command", value: "/clear", action: "clear" },
        { label: "/help", hint: qsTr("Insert a help request"), group: "Command", value: "/help ", action: "insert" }
    ]

    readonly property var mentions: [
        { label: "README.md", hint: qsTr("Project readme"), group: "File", value: "@file:README.md ", action: "insert" },
        { label: "Composer.qml", hint: qsTr("Composer component"), group: "File", value: "@file:Composer.qml ", action: "insert" },
        { label: "Theme.qml", hint: qsTr("Theme tokens"), group: "File", value: "@file:Theme.qml ", action: "insert" }
    ]

    // Filter the relevant pool by a case-insensitive substring of the query.
    function search(kind, query) {
        var pool = kind === "mention" ? mentions : slashCommands;
        var q = String(query || "").toLowerCase();

        if (q.length === 0)
            return pool.slice(0, 8);

        var out = [];
        for (var i = 0; i < pool.length; ++i) {
            if (pool[i].label.toLowerCase().indexOf(q) >= 0)
                out.push(pool[i]);
        }
        return out.slice(0, 8);
    }
}
