#include "line_editor.h"

#include <QDir>
#include <QFile>
#include <QProcessEnvironment>

namespace lineedit {
namespace {

bool isWordChar(QChar c) {
    // readline's default word boundary: alphanumeric only (underscore is NOT a
    // word char in the stock emacs keymap).
    return c.isLetterOrNumber();
}

int forwardWordIndex(const QString& text, int i) {
    const int n = static_cast<int>(text.size());
    while (i < n && !isWordChar(text.at(i))) {
        ++i;
    }
    while (i < n && isWordChar(text.at(i))) {
        ++i;
    }
    return i;
}

int backwardWordIndex(const QString& text, int i) {
    while (i > 0 && !isWordChar(text.at(i - 1))) {
        --i;
    }
    while (i > 0 && isWordChar(text.at(i - 1))) {
        --i;
    }
    return i;
}

// unix-word-rubout uses whitespace (not word-char) as the only delimiter.
int backwardWhitespaceIndex(const QString& text, int i) {
    while (i > 0 && text.at(i - 1).isSpace()) {
        --i;
    }
    while (i > 0 && !text.at(i - 1).isSpace()) {
        --i;
    }
    return i;
}

const QHash<QString, EditCommand>& functionTable() {
    static const QHash<QString, EditCommand> table = {
        {QStringLiteral("beginning-of-line"), EditCommand::BeginningOfLine},
        {QStringLiteral("end-of-line"), EditCommand::EndOfLine},
        {QStringLiteral("forward-char"), EditCommand::ForwardChar},
        {QStringLiteral("backward-char"), EditCommand::BackwardChar},
        {QStringLiteral("forward-word"), EditCommand::ForwardWord},
        {QStringLiteral("backward-word"), EditCommand::BackwardWord},
        {QStringLiteral("delete-char"), EditCommand::DeleteChar},
        {QStringLiteral("backward-delete-char"), EditCommand::BackwardDeleteChar},
        {QStringLiteral("kill-line"), EditCommand::KillLine},
        {QStringLiteral("unix-line-discard"), EditCommand::UnixLineDiscard},
        {QStringLiteral("kill-word"), EditCommand::KillWord},
        {QStringLiteral("backward-kill-word"), EditCommand::BackwardKillWord},
        {QStringLiteral("unix-word-rubout"), EditCommand::UnixWordRubout},
        {QStringLiteral("yank"), EditCommand::Yank},
        {QStringLiteral("yank-pop"), EditCommand::YankPop},
        {QStringLiteral("transpose-chars"), EditCommand::TransposeChars},
    };
    return table;
}

// A single resolved key chord.
struct Chord {
    int key = 0;
    Qt::KeyboardModifiers mods = Qt::NoModifier;
    bool ok = false;
};

// Map a base character (plus accumulated Ctrl/Alt flags) to a Qt key chord.
Chord chordForChar(QChar base, bool ctrl, bool meta) {
    Chord c;
    c.mods =
        (ctrl ? Qt::ControlModifier : Qt::NoModifier) | (meta ? Qt::AltModifier : Qt::NoModifier);
    const QChar up = base.toUpper();
    if (up >= QLatin1Char('A') && up <= QLatin1Char('Z')) {
        c.key = Qt::Key_A + (up.unicode() - u'A');
        c.ok = true;
    } else if (base >= QLatin1Char('0') && base <= QLatin1Char('9')) {
        c.key = Qt::Key_0 + (base.unicode() - u'0');
        c.ok = true;
    } else if (base == QLatin1Char(' ')) {
        c.key = Qt::Key_Space;
        c.ok = true;
    }
    return c;
}

// Parse a quoted readline key sequence (the part inside the quotes). Returns a
// single chord; multi-chord sequences (e.g. "\C-x\C-e") are rejected (ok=false).
Chord parseQuotedSeq(const QString& s) {
    Chord result;
    bool ctrl = false;
    bool meta = false;
    bool sawChord = false;
    int i = 0;
    const int n = static_cast<int>(s.size());
    while (i < n) {
        QChar ch = s.at(i);
        if (ch == QLatin1Char('\\') && i + 1 < n) {
            const QChar next = s.at(i + 1);
            if ((next == QLatin1Char('C') || next == QLatin1Char('c')) && i + 2 < n &&
                s.at(i + 2) == QLatin1Char('-')) {
                ctrl = true;
                i += 3;
                continue;
            }
            if ((next == QLatin1Char('M') || next == QLatin1Char('m')) && i + 2 < n &&
                s.at(i + 2) == QLatin1Char('-')) {
                meta = true;
                i += 3;
                continue;
            }
            if (next == QLatin1Char('e')) { // ESC acts as a Meta prefix
                meta = true;
                i += 2;
                continue;
            }
            // A literal escaped character (\\, \" ...). \t/\r/\n are not supported
            // as chord bases here.
            if (next == QLatin1Char('t') || next == QLatin1Char('r') || next == QLatin1Char('n')) {
                return {};
            }
            if (sawChord) {
                return {};
            }
            result = chordForChar(next, ctrl, meta);
            sawChord = true;
            ctrl = meta = false;
            i += 2;
            continue;
        }
        if (sawChord) {
            return {}; // more than one chord -> unsupported
        }
        result = chordForChar(ch, ctrl, meta);
        sawChord = true;
        ctrl = meta = false;
        ++i;
    }
    return sawChord ? result : Chord{};
}

// Parse the unquoted symbolic form: "Control-a", "Meta-b", "C-M-x", etc.
Chord parseSymbolicSeq(const QString& s) {
    const QStringList parts = s.split(QLatin1Char('-'), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return {};
    }
    bool ctrl = false;
    bool meta = false;
    for (int i = 0; i < parts.size() - 1; ++i) {
        const QString p = parts.at(i).toLower();
        if (p == QStringLiteral("control") || p == QStringLiteral("ctrl") ||
            p == QStringLiteral("c")) {
            ctrl = true;
        } else if (p == QStringLiteral("meta") || p == QStringLiteral("alt") ||
                   p == QStringLiteral("m")) {
            meta = true;
        } else {
            return {}; // unknown modifier token
        }
    }
    const QString& last = parts.last();
    if (last.size() == 1) {
        return chordForChar(last.at(0), ctrl, meta);
    }
    const QString low = last.toLower();
    if (low == QStringLiteral("space")) {
        return chordForChar(QLatin1Char(' '), ctrl, meta);
    }
    return {};
}

Chord parseKeyseq(const QString& raw) {
    const QString trimmed = raw.trimmed();
    if (trimmed.size() >= 2 && trimmed.startsWith(QLatin1Char('"')) &&
        trimmed.endsWith(QLatin1Char('"'))) {
        return parseQuotedSeq(trimmed.mid(1, trimmed.size() - 2));
    }
    return parseSymbolicSeq(trimmed);
}

} // namespace

void KillRing::pushKill(const QString& killed, bool backward, bool merge) {
    if (killed.isEmpty()) {
        return;
    }
    if (merge && !m_ring.isEmpty()) {
        if (backward) {
            m_ring.front().prepend(killed);
        } else {
            m_ring.front().append(killed);
        }
    } else {
        m_ring.prepend(killed);
    }
    m_yankIndex = 0;
}

QString KillRing::beginYank() {
    m_yankIndex = 0;
    return m_ring.isEmpty() ? QString() : m_ring.front();
}

QString KillRing::rotate() {
    if (m_ring.isEmpty()) {
        return {};
    }
    m_yankIndex = (m_yankIndex + 1) % static_cast<int>(m_ring.size());
    return m_ring.at(m_yankIndex);
}

void KillRing::endCommand(bool didKill, bool didYank) {
    m_lastWasKill = didKill;
    m_lastWasYank = didYank;
}

bool LineEditor::applyCommand(EditCommand cmd, QString& text, int& cursor, KillRing& ring) {
    cursor = qBound(0, cursor, static_cast<int>(text.size()));
    const bool wasKill = ring.lastWasKill();
    const bool wasYank = ring.lastWasYank();
    bool didKill = false;
    bool didYank = false;
    bool handled = true;

    switch (cmd) {
    case EditCommand::None:
        handled = false;
        break;
    case EditCommand::BeginningOfLine:
        cursor = 0;
        break;
    case EditCommand::EndOfLine:
        cursor = static_cast<int>(text.size());
        break;
    case EditCommand::ForwardChar:
        if (cursor < text.size()) {
            ++cursor;
        }
        break;
    case EditCommand::BackwardChar:
        if (cursor > 0) {
            --cursor;
        }
        break;
    case EditCommand::ForwardWord:
        cursor = forwardWordIndex(text, cursor);
        break;
    case EditCommand::BackwardWord:
        cursor = backwardWordIndex(text, cursor);
        break;
    case EditCommand::DeleteChar:
        if (cursor < text.size()) {
            text.remove(cursor, 1);
        }
        break;
    case EditCommand::BackwardDeleteChar:
        if (cursor > 0) {
            text.remove(cursor - 1, 1);
            --cursor;
        }
        break;
    case EditCommand::KillLine: {
        const QString killed = text.mid(cursor);
        text.truncate(cursor);
        ring.pushKill(killed, false, wasKill);
        didKill = true;
        break;
    }
    case EditCommand::UnixLineDiscard: {
        const QString killed = text.left(cursor);
        text.remove(0, cursor);
        ring.pushKill(killed, true, wasKill);
        cursor = 0;
        didKill = true;
        break;
    }
    case EditCommand::KillWord: {
        const int end = forwardWordIndex(text, cursor);
        const QString killed = text.mid(cursor, end - cursor);
        text.remove(cursor, end - cursor);
        ring.pushKill(killed, false, wasKill);
        didKill = true;
        break;
    }
    case EditCommand::BackwardKillWord: {
        const int start = backwardWordIndex(text, cursor);
        const QString killed = text.mid(start, cursor - start);
        text.remove(start, cursor - start);
        ring.pushKill(killed, true, wasKill);
        cursor = start;
        didKill = true;
        break;
    }
    case EditCommand::UnixWordRubout: {
        const int start = backwardWhitespaceIndex(text, cursor);
        const QString killed = text.mid(start, cursor - start);
        text.remove(start, cursor - start);
        ring.pushKill(killed, true, wasKill);
        cursor = start;
        didKill = true;
        break;
    }
    case EditCommand::Yank: {
        const QString s = ring.beginYank();
        if (!s.isEmpty()) {
            text.insert(cursor, s);
            ring.setYankSpan(cursor, static_cast<int>(s.size()));
            cursor += static_cast<int>(s.size());
        }
        didYank = true;
        break;
    }
    case EditCommand::YankPop: {
        if (!wasYank || ring.size() < 2) {
            handled = false;
            break;
        }
        const QString next = ring.rotate();
        text.remove(ring.yankPos(), ring.yankLen());
        text.insert(ring.yankPos(), next);
        cursor = ring.yankPos() + static_cast<int>(next.size());
        ring.setYankSpan(ring.yankPos(), static_cast<int>(next.size()));
        didYank = true;
        break;
    }
    case EditCommand::TransposeChars: {
        const int len = static_cast<int>(text.size());
        if (len >= 2) {
            int c = cursor;
            if (c == 0) {
                c = 1;
            }
            if (c >= len) {
                c = len - 1;
            }
            const QChar tmp = text.at(c - 1);
            text[c - 1] = text.at(c);
            text[c] = tmp;
            cursor = qMin(c + 1, len);
        }
        break;
    }
    }

    ring.endCommand(didKill, didYank);
    return handled;
}

quint64 LineEditor::encode(int key, Qt::KeyboardModifiers mods) {
    const Qt::KeyboardModifiers kept =
        mods & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier);
    return (static_cast<quint64>(static_cast<unsigned>(kept)) << 32) | static_cast<quint32>(key);
}

EditCommand LineEditor::commandForFunction(const QString& name) {
    return functionTable().value(name.trimmed(), EditCommand::None);
}

QHash<quint64, EditCommand> LineEditor::defaultKeymap() {
    QHash<quint64, EditCommand> m;
    const auto bind = [&](int key, Qt::KeyboardModifiers mods, EditCommand cmd) {
        m.insert(encode(key, mods), cmd);
    };
    bind(Qt::Key_A, Qt::ControlModifier, EditCommand::BeginningOfLine);
    bind(Qt::Key_E, Qt::ControlModifier, EditCommand::EndOfLine);
    bind(Qt::Key_F, Qt::ControlModifier, EditCommand::ForwardChar);
    bind(Qt::Key_B, Qt::ControlModifier, EditCommand::BackwardChar);
    bind(Qt::Key_F, Qt::AltModifier, EditCommand::ForwardWord);
    bind(Qt::Key_B, Qt::AltModifier, EditCommand::BackwardWord);
    bind(Qt::Key_D, Qt::ControlModifier, EditCommand::DeleteChar);
    bind(Qt::Key_K, Qt::ControlModifier, EditCommand::KillLine);
    bind(Qt::Key_U, Qt::ControlModifier, EditCommand::UnixLineDiscard);
    bind(Qt::Key_D, Qt::AltModifier, EditCommand::KillWord);
    bind(Qt::Key_W, Qt::ControlModifier, EditCommand::UnixWordRubout);
    bind(Qt::Key_Backspace, Qt::AltModifier, EditCommand::BackwardKillWord);
    bind(Qt::Key_Y, Qt::ControlModifier, EditCommand::Yank);
    bind(Qt::Key_Y, Qt::AltModifier, EditCommand::YankPop);
    bind(Qt::Key_T, Qt::ControlModifier, EditCommand::TransposeChars);
    return m;
}

void LineEditor::parseInputrcInto(QHash<quint64, EditCommand>& map, const QString& contents) {
    // A stack of conditional frames; a frame is `true` when its block is being
    // skipped (e.g. `$if mode=vi`). We ignore bindings while any frame is active.
    QList<bool> condStack;
    const auto skipping = [&] { return condStack.contains(true); };

    const QStringList lines = contents.split(QLatin1Char('\n'));
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (line.startsWith(QStringLiteral("$if"))) {
            const QString cond = line.mid(3).trimmed().toLower();
            // Skip mode-conditional blocks that are not emacs; descend into the rest.
            const bool skip =
                cond.startsWith(QStringLiteral("mode=")) && !cond.contains(QStringLiteral("emacs"));
            condStack.append(skip);
            continue;
        }
        if (line.startsWith(QStringLiteral("$endif"))) {
            if (!condStack.isEmpty()) {
                condStack.removeLast();
            }
            continue;
        }
        if (line.startsWith(QStringLiteral("$else"))) {
            if (!condStack.isEmpty()) {
                condStack.last() = !condStack.last();
            }
            continue;
        }
        if (skipping()) {
            continue;
        }
        if (line.startsWith(QStringLiteral("set "))) {
            // We only support the emacs editing-mode; ignore everything else.
            continue;
        }

        const int colon = static_cast<int>(line.indexOf(QLatin1Char(':')));
        if (colon <= 0) {
            continue;
        }
        const QString lhs = line.left(colon).trimmed();
        const QString rhs = line.mid(colon + 1).trimmed();
        if (rhs.startsWith(QLatin1Char('"'))) {
            continue; // a macro binding (keyseq -> string); unsupported
        }
        const EditCommand cmd = commandForFunction(rhs);
        if (cmd == EditCommand::None) {
            continue; // function we do not implement
        }
        const Chord chord = parseKeyseq(lhs);
        if (!chord.ok) {
            continue;
        }
        map.insert(encode(chord.key, chord.mods), cmd);
    }
}

const QHash<quint64, EditCommand>& LineEditor::keymap() {
    static const QHash<quint64, EditCommand> cached = [] {
        QHash<quint64, EditCommand> m = defaultKeymap();
        const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString path = env.value(QStringLiteral("INPUTRC"));
        if (path.isEmpty()) {
            const QString home = QDir::homePath();
            if (!home.isEmpty()) {
                path = home + QStringLiteral("/.inputrc");
            }
        }
        if (!path.isEmpty()) {
            QFile f(path);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                parseInputrcInto(m, QString::fromUtf8(f.readAll()));
            }
        }
        return m;
    }();
    return cached;
}

EditCommand LineEditor::lookup(int key, Qt::KeyboardModifiers mods) {
    return keymap().value(encode(key, mods), EditCommand::None);
}

EditCommand LineEditor::lookupEvent(int key, Qt::KeyboardModifiers mods, const QString& text) {
    const EditCommand direct = lookup(key, mods);
    if (direct != EditCommand::None) {
        return direct;
    }
    // Ctrl/Alt+letter arrive as char events (key()==Key_unknown, letter in text()).
    if (text.size() == 1) {
        const QChar ch = text.at(0);
        const QChar up = ch.toUpper();
        int derived = 0;
        if (up >= QLatin1Char('A') && up <= QLatin1Char('Z')) {
            derived = Qt::Key_A + (up.unicode() - u'A');
        } else if (ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) {
            derived = Qt::Key_0 + (ch.unicode() - u'0');
        }
        if (derived != 0) {
            return lookup(derived, mods);
        }
    }
    return EditCommand::None;
}

} // namespace lineedit
