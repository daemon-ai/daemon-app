// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "root_widget_detail.h"

#include "tab_model.h"

#include <cstdio>
#include <QLatin1Char>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>

namespace rwdetail {

void emitDesktopNotification(const QString& title, const QString& body) {
    const QString notifySend = QStandardPaths::findExecutable(QStringLiteral("notify-send"));
    if (!notifySend.isEmpty()) {
        QProcess::startDetached(notifySend, QStringList{title, body});
    }
    // OSC 9 (iTerm2-style growl): ESC ] 9 ; <text> BEL.
    const QString msg = body.isEmpty() ? title : (title + QStringLiteral(" \u2014 ") + body);
    std::fputs(QStringLiteral("\033]9;%1\a").arg(msg).toUtf8().constData(), stdout);
    std::fflush(stdout);
}

QString titleForContent(const QString& markdown) {
    const QString trimmed = markdown.trimmed();
    if (trimmed.isEmpty()) {
        return QObject::tr("New session");
    }
    QString first = trimmed.section(QLatin1Char('\n'), 0, 0);
    while (first.startsWith(QLatin1Char('#'))) {
        first.remove(0, 1);
    }
    first = first.trimmed();
    if (first.isEmpty()) {
        return QObject::tr("Session");
    }
    return first.left(24);
}

QString pageMarkdown(int kind) {
    if (kind == TabModel::Settings) {
        return QObject::tr(
            "# Settings\n\n"
            "A generic, non-transcript page hosted by the same tab strip.\n\n"
            "- Press **F8** to cycle the theme (Light / Dark / Sepia / Midnight)\n"
            "- **Ctrl+T** opens a new transcript tab\n"
            "- **Ctrl+W** closes the current tab\n"
            "- **Ctrl+Tab** / **Ctrl+Shift+Tab** switch tabs (wrapping)\n"
            "- **Alt+1..9** jumps directly to tab N\n"
            "- **Tab** cycles panes, including the tab strip; when it is focused, "
            "**Left**/**Right** switch tabs, **Enter** pins a preview, **Delete** closes\n"
            "- In an interactive block (approval / clarify): **Up**/**Down**/**Left**/"
            "**Right** move between controls, **Space** toggles, **Enter** submits, "
            "**Tab** leaves to the next pane\n"
            "- In the transcript: **r** opens the rewind picker over your prior "
            "messages (**Up**/**Down** to choose, **Enter** restores & re-runs, "
            "**e** edits in the composer, **Esc** cancels)\n"
            "- **/retry**, **/edit**, **/undo** rewind the last message from the "
            "composer\n");
    }
    return {};
}

} // namespace rwdetail
