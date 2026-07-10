// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/autostart/autostart_entry.h"

namespace autostart {

namespace {

// Split an Exec= value into unquoted tokens per the desktop-entry spec's
// quoting rules (double quotes, backslash escapes inside). Tolerant: a
// malformed tail yields the tokens parsed so far.
QStringList tokenizeExec(const QString& value) {
    QStringList tokens;
    QString current;
    bool inQuotes = false;
    bool haveToken = false;
    for (int i = 0; i < value.size(); ++i) {
        const QChar c = value.at(i);
        if (inQuotes) {
            if (c == QLatin1Char('\\') && i + 1 < value.size()) {
                current += value.at(++i);
            } else if (c == QLatin1Char('"')) {
                inQuotes = false;
            } else {
                current += c;
            }
            continue;
        }
        if (c == QLatin1Char('"')) {
            inQuotes = true;
            haveToken = true;
            continue;
        }
        if (c == QLatin1Char(' ') || c == QLatin1Char('\t')) {
            if (haveToken || !current.isEmpty()) {
                tokens << current;
                current.clear();
                haveToken = false;
            }
            continue;
        }
        current += c;
        haveToken = true;
    }
    if (haveToken || !current.isEmpty()) {
        tokens << current;
    }
    return tokens;
}

bool valueIsTrue(const QString& value) {
    return value.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
}

} // namespace

QString quoteExecArg(const QString& arg) {
    QString quoted;
    quoted.reserve(arg.size() + 2);
    quoted += QLatin1Char('"');
    for (const QChar c : arg) {
        // The spec reserves `"` ` (backtick) `$` `\` inside a quoted argument.
        if (c == QLatin1Char('"') || c == QLatin1Char('`') || c == QLatin1Char('$') ||
            c == QLatin1Char('\\')) {
            quoted += QLatin1Char('\\');
        }
        quoted += c;
    }
    quoted += QLatin1Char('"');
    return quoted;
}

QString renderAutostartEntry(const QString& program, const QStringList& args, bool hidden,
                             bool gnomeEnabled) {
    QStringList exec;
    exec << quoteExecArg(program);
    for (const QString& arg : args) {
        exec << quoteExecArg(arg);
    }
    QString out;
    out += QStringLiteral("# Written by Daemon (Settings > Advanced > Startup); safe to delete.\n");
    out += QStringLiteral("[Desktop Entry]\n");
    out += QStringLiteral("Type=Application\n");
    out += QStringLiteral("Name=Daemon\n");
    // TryExec is a plain path (no Exec quoting): the DE silently skips the
    // entry when the binary vanished instead of erroring at login.
    out += QStringLiteral("TryExec=") + program + QLatin1Char('\n');
    out += QStringLiteral("Exec=") + exec.join(QLatin1Char(' ')) + QLatin1Char('\n');
    out += QStringLiteral("Icon=daemon-app\n");
    out += QStringLiteral("Terminal=false\n");
    out += QStringLiteral("StartupNotify=false\n");
    out += QStringLiteral("X-GNOME-Autostart-enabled=") +
           (gnomeEnabled ? QStringLiteral("true") : QStringLiteral("false")) + QLatin1Char('\n');
    if (hidden) {
        out += QStringLiteral("Hidden=true\n");
    }
    return out;
}

EntryFacts parseAutostartEntry(const QString& content) {
    EntryFacts facts;
    const QStringList lines = content.split(QLatin1Char('\n'));
    for (const QString& raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')) ||
            line.startsWith(QLatin1Char('['))) {
            continue;
        }
        const int eq = static_cast<int>(line.indexOf(QLatin1Char('=')));
        if (eq <= 0) {
            continue;
        }
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        if (key == QLatin1String("Exec")) {
            const QStringList tokens = tokenizeExec(value);
            facts.execProgram = tokens.value(0);
            facts.execArgs = tokens.mid(1);
        } else if (key == QLatin1String("TryExec")) {
            facts.tryExec = value;
        } else if (key == QLatin1String("Hidden")) {
            facts.hidden = valueIsTrue(value);
        } else if (key == QLatin1String("X-GNOME-Autostart-enabled")) {
            facts.gnomeEnabled = valueIsTrue(value);
        }
    }
    return facts;
}

bool entryEnablesAutostart(const EntryFacts& facts) {
    return !facts.execProgram.isEmpty() && !facts.hidden && facts.gnomeEnabled;
}

bool entryNeedsRewrite(const EntryFacts& facts, const QString& program, const QString& hiddenArg) {
    return facts.execProgram != program || facts.tryExec != program ||
           !facts.execArgs.contains(hiddenArg);
}

} // namespace autostart
