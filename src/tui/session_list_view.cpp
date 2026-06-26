#include "session_list_view.h"

#include "presentation/display_presenter.h"
#include "sessions_list_model.h"
#include "tui_palette.h"

#include <QDateTime>
#include <QLocale>
#include <QRect>
#include <QStringList>
#include <Tui/ZColor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

namespace {

Span mkSpan(const QString& text, const Tui::ZColor& fg, const Tui::ZColor& bg,
            Tui::ZTextAttributes attr = {}) {
    return Span{text, fg, bg, attr};
}

// Single-line elide to `width` columns with a trailing ellipsis.
QString elide(const QString& text, int width) {
    if (width <= 0) {
        return QString();
    }
    if (text.size() <= width) {
        return text;
    }
    if (width == 1) {
        return QStringLiteral("\u2026");
    }
    return text.left(width - 1) + QStringLiteral("\u2026");
}

} // namespace

SessionListView::SessionListView(Tui::ZWidget* parent) : Tui::ZWidget(parent) {
    setFocusPolicy(Tui::StrongFocus);
    setSizePolicyV(Tui::SizePolicy::Expanding);
}

void SessionListView::setModel(SessionsListModel* model) {
    m_model = model;
    if (m_model != nullptr) {
        const auto repaint = [this] { rebuild(); };
        connect(m_model, &QAbstractItemModel::modelReset, this, repaint);
        connect(m_model, &QAbstractItemModel::dataChanged, this, repaint);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, repaint);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, repaint);
        connect(m_model, &SessionsListModel::selectionChanged, this, [this](const QString&) {
            rebuild();
            ensureVisible(m_model->currentRow());
            update();
        });
    }
    rebuild();
}

void SessionListView::relayout() {
    rebuild();
}

int SessionListView::rowAt(int localY) const {
    const int idx = m_scrollTop + localY;
    if (idx < 0 || idx >= static_cast<int>(m_rowOfLine.size())) {
        return -1;
    }
    return m_rowOfLine.at(idx); // -1 on the gap line between cards
}

void SessionListView::activateAtLocalY(int localY) {
    const int row = rowAt(localY);
    if (row >= 0) {
        // A single click previews (transient); double-click/Enter pins.
        emit rowActivated(row, false);
    }
}

void SessionListView::scrollByLines(int delta) {
    m_scrollTop += delta;
    clampScrollTop();
    update();
}

int SessionListView::visibleRows() const {
    return qMax(0, geometry().height());
}

int SessionListView::maxScrollTop() const {
    return qMax(0, static_cast<int>(m_lines.size()) - visibleRows());
}

void SessionListView::clampScrollTop() {
    m_scrollTop = qBound(0, m_scrollTop, maxScrollTop());
}

void SessionListView::ensureVisible(int row) {
    if (row < 0 || row >= static_cast<int>(m_lineOfRow.size())) {
        return;
    }
    const int first = m_lineOfRow.at(row);
    const int next = (row + 1 < static_cast<int>(m_lineOfRow.size()))
                         ? m_lineOfRow.at(row + 1)
                         : static_cast<int>(m_lines.size());
    const int last = qMax(first, next - 2); // exclude the trailing gap line
    const int h = visibleRows();
    if (first < m_scrollTop) {
        m_scrollTop = first;
    } else if (last >= m_scrollTop + h) {
        m_scrollTop = last - h + 1;
    }
    clampScrollTop();
}

void SessionListView::rebuild() {
    m_lines.clear();
    m_rowOfLine.clear();
    m_lineOfRow.clear();
    if (m_model == nullptr) {
        update();
        return;
    }

    const int w = qMax(1, geometry().width());
    const int contentW = qMax(1, w - 1); // reserve a column for the scrollbar
    const int rows = m_model->rowCount();

    const auto push = [this](const RenderLine& line, int row) {
        m_lines.push_back(line);
        m_rowOfLine.push_back(row);
    };

    for (int row = 0; row < rows; ++row) {
        m_lineOfRow.push_back(static_cast<int>(m_lines.size()));
        const QModelIndex idx = m_model->index(row, 0);
        const auto role = [&](int r) { return m_model->data(idx, r); };

        const bool selected = role(SessionsListModel::CurrentRole).toBool();
        const QString title = role(SessionsListModel::TitleRole).toString();
        const QString snippet = role(SessionsListModel::SnippetRole).toString();
        const QDateTime modified = role(SessionsListModel::ModifiedRole).toDateTime();
        const QString agent = role(SessionsListModel::UnitNameRole).toString();
        const QString kind =
            DisplayPresenter::agentKindIconKeyFor(role(SessionsListModel::UnitKindRole).toInt());
        const QStringList tagNames = role(SessionsListModel::TagNamesRole).toStringList();
        const QStringList tagColors = role(SessionsListModel::TagColorsRole).toStringList();

        const Tui::ZColor titleFg = selected ? tpal::accent() : tpal::fg();

        // Line 1: " title .... timestamp" (timestamp right-aligned, muted).
        {
            // Locale-aware short date/time so the stamp follows the active UI
            // language's conventions rather than a fixed English format.
            const QString stamp =
                modified.isValid() ? QLocale().toString(modified, QLocale::ShortFormat) : QString();
            const int stampW = static_cast<int>(stamp.size());
            // 1 leading space + title + at least 2 gap + stamp.
            const int titleBudget = qMax(1, contentW - 1 - stampW - 2);
            const QString shownTitle = elide(title, titleBudget);
            RenderLine line;
            line.push_back(mkSpan(QStringLiteral(" "), titleFg, tpal::bg()));
            line.push_back(mkSpan(shownTitle, titleFg, tpal::bg(), Tui::ZTextAttribute::Bold));
            if (!stamp.isEmpty()) {
                const int used = 1 + static_cast<int>(shownTitle.size());
                const int pad = qMax(1, contentW - used - stampW);
                line.push_back(mkSpan(QString(pad, QLatin1Char(' ')), tpal::muted(), tpal::bg()));
                line.push_back(mkSpan(stamp, tpal::muted(), tpal::bg()));
            }
            push(line, row);
        }

        // Line 2: snippet (muted, elided), omitted when empty.
        if (!snippet.isEmpty()) {
            RenderLine line;
            line.push_back(mkSpan(QStringLiteral("   "), tpal::muted(), tpal::bg()));
            line.push_back(mkSpan(elide(snippet, contentW - 3), tpal::muted(), tpal::bg()));
            push(line, row);
        }

        // Line 3: agent-kind glyph + agent name + tag dots, omitted when empty.
        if (!agent.isEmpty() || !tagNames.isEmpty()) {
            RenderLine line;
            line.push_back(mkSpan(QStringLiteral("   "), tpal::faint(), tpal::bg()));
            if (!agent.isEmpty()) {
                line.push_back(mkSpan(tpal::agentKindGlyph(kind) + QStringLiteral(" "),
                                      tpal::accent(), tpal::bg()));
                line.push_back(mkSpan(agent, tpal::faint(), tpal::bg()));
            }
            for (int t = 0; t < tagNames.size(); ++t) {
                // Parse the tag's "#rrggbb" color directly to an RGB ZColor.
                Tui::ZColor tagFg = tpal::muted();
                if (t < tagColors.size()) {
                    const QString hex = tagColors.at(t);
                    if (hex.size() == 7 && hex.startsWith(QLatin1Char('#'))) {
                        bool ok = false;
                        const int rgb = hex.mid(1).toInt(&ok, 16);
                        if (ok) {
                            tagFg = Tui::ZColor((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
                        }
                    }
                }
                line.push_back(mkSpan(QStringLiteral("  ") + tpal::tagDot() + QStringLiteral(" "),
                                      tagFg, tpal::bg()));
                line.push_back(mkSpan(tagNames.at(t), tpal::faint(), tpal::bg()));
            }
            push(line, row);
        }

        // Gap line between cards.
        push(RenderLine{}, -1);
    }

    clampScrollTop();
    update();
}

void SessionListView::resizeEvent(Tui::ZResizeEvent* event) {
    Tui::ZWidget::resizeEvent(event);
    rebuild();
    if (m_model != nullptr) {
        ensureVisible(m_model->currentRow());
    }
}

void SessionListView::paintEvent(Tui::ZPaintEvent* event) {
    Tui::ZPainter* p = event->painter();
    const Tui::ZColor pageFg = tpal::fg();
    const Tui::ZColor pageBg = tpal::bg();
    p->clear(pageFg, pageBg);

    const int h = geometry().height();
    const int w = geometry().width();
    const int lineCount = static_cast<int>(m_lines.size());
    const bool scrollable = lineCount > h;
    const int contentW = scrollable ? qMax(1, w - 1) : w;
    const int selectedRow = (m_model != nullptr) ? m_model->currentRow() : -1;

    for (int rowY = 0; rowY < h; ++rowY) {
        const int idx = m_scrollTop + rowY;
        if (idx < 0 || idx >= lineCount) {
            continue;
        }
        const int srcRow = m_rowOfLine.at(idx);
        const bool washed = srcRow >= 0 && srcRow == selectedRow;
        // The selected row uses a brighter wash when the column is focused and a
        // weaker one when it is not, so "this column is focused" reads at a glance.
        const Tui::ZColor washBg = focus() ? tpal::selectionBg() : tpal::selectionInactiveBg();
        const Tui::ZColor rowBg = washed ? washBg : pageBg;
        if (washed) {
            p->clearRect(0, rowY, contentW, 1, pageFg, rowBg);
        }

        int x = 0;
        for (const Span& s : m_lines.at(idx)) {
            if (x >= contentW) {
                break;
            }
            const int len = static_cast<int>(s.text.size());
            QString text = s.text;
            if (x + len > contentW) {
                text = text.left(contentW - x);
            }
            if (!text.isEmpty()) {
                const Tui::ZColor bg = washed ? rowBg : s.bg;
                if (s.attr != Tui::ZTextAttributes{}) {
                    p->writeWithAttributes(x, rowY, text, s.fg, bg, s.attr);
                } else {
                    p->writeWithColors(x, rowY, text, s.fg, bg);
                }
            }
            x += len;
        }
    }

    if (scrollable && h > 0) {
        const int total = lineCount;
        int thumbLen = qMax(1, (h * h) / total);
        int thumbTop = (m_scrollTop * h) / total;
        thumbTop = qBound(0, thumbTop, h - thumbLen);
        const int xCol = w - 1;
        for (int rowY = 0; rowY < h; ++rowY) {
            const bool onThumb = rowY >= thumbTop && rowY < thumbTop + thumbLen;
            p->writeWithColors(xCol, rowY,
                               onThumb ? QStringLiteral("\u2503") : QStringLiteral("\u2502"),
                               onThumb ? tpal::accent() : tpal::muted(), pageBg);
        }
    }
}

void SessionListView::keyEvent(Tui::ZKeyEvent* event) {
    // Ctrl-modified session actions on the current row (rename/export/pin-toggle).
    // Tui delivers Ctrl+letter as a char event (text()==letter), so match text.
    if (m_model != nullptr && (event->modifiers() & Qt::ControlModifier)) {
        const int row = m_model->currentRow();
        const QString t = event->text();
        if (row >= 0 && t.compare(QStringLiteral("r"), Qt::CaseInsensitive) == 0) {
            emit renameRequested(row);
            event->accept();
            return;
        }
        if (row >= 0 && t.compare(QStringLiteral("e"), Qt::CaseInsensitive) == 0) {
            emit exportRequested(row);
            event->accept();
            return;
        }
        if (row >= 0 && t.compare(QStringLiteral("k"), Qt::CaseInsensitive) == 0) {
            emit pinToggleRequested(row);
            event->accept();
            return;
        }
    }

    // Alt+Up / Alt+Down reorders the current row within the list (mirrors the GUI
    // "Move up"/"Move down" context-menu actions).
    if (m_model != nullptr && (event->modifiers() & Qt::AltModifier)) {
        const int row = m_model->currentRow();
        if (row >= 0 && event->key() == Qt::Key_Up) {
            emit moveRequested(row, -1);
            event->accept();
            return;
        }
        if (row >= 0 && event->key() == Qt::Key_Down) {
            emit moveRequested(row, 1);
            event->accept();
            return;
        }
    }

    if (m_model != nullptr &&
        (event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::ShiftModifier)) {
        const int key = event->key();
        const Qt::KeyboardModifiers mods = event->modifiers();

        // Card navigation (arrows / Home / End) and open (Enter). Only on the plain
        // (no Shift) variants so Shift+printables still type into the query.
        if (mods == Qt::NoModifier) {
            bool navHandled = true;
            switch (key) {
            case Qt::Key_Up:
                m_model->selectPrevious();
                emit rowActivated(m_model->currentRow(), false); // preview
                break;
            case Qt::Key_Down:
                m_model->selectNext();
                emit rowActivated(m_model->currentRow(), false); // preview
                break;
            case Qt::Key_Home:
                if (m_model->rowCount() > 0) {
                    m_model->activate(0);
                    emit rowActivated(0, false); // preview
                }
                break;
            case Qt::Key_End:
                if (m_model->rowCount() > 0) {
                    m_model->activate(m_model->rowCount() - 1);
                    emit rowActivated(m_model->rowCount() - 1, false); // preview
                }
                break;
            case Qt::Key_Enter:
            case Qt::Key_Return:
                if (m_model->currentRow() >= 0) {
                    emit rowActivated(m_model->currentRow(), true); // pinned open
                }
                break;
            case Qt::Key_Delete:
                if (m_model->currentRow() >= 0) {
                    emit deleteRequested(m_model->currentRow());
                }
                break;
            default:
                navHandled = false;
                break;
            }
            if (navHandled) {
                event->accept();
                return;
            }

            // Esc clears an active query first; only an empty query bubbles up (to
            // the shell's context-sensitive quit chain).
            if (key == Qt::Key_Escape) {
                if (!m_model->search().isEmpty()) {
                    emit searchClear();
                    event->accept();
                    return;
                }
            } else if (key == Qt::Key_Backspace) {
                emit searchBackspace();
                event->accept();
                return;
            }
        }

        // Type-ahead: any printable character builds the search query (the model
        // filters live; selection re-anchors to the first match in RootWidget).
        const QString text = event->text();
        if (!text.isEmpty() && text.at(0).isPrint()) {
            emit searchAppend(text);
            event->accept();
            return;
        }
    }
    // Everything else (notably an empty-query Esc) bubbles up unhandled.
    Tui::ZWidget::keyEvent(event);
}

void SessionListView::focusInEvent(Tui::ZFocusEvent* event) {
    Tui::ZWidget::focusInEvent(event);
    // Entering the list with nothing selected: anchor on the first card so a single
    // Up/Down isn't "spent" just selecting row 0, and Enter works immediately.
    if (m_model != nullptr && m_model->currentRow() < 0 && m_model->rowCount() > 0) {
        m_model->activate(0);
    }
    emit focusChanged(true);
    update(); // repaint the selection wash in its focused (brighter) tone
}

void SessionListView::focusOutEvent(Tui::ZFocusEvent* event) {
    Tui::ZWidget::focusOutEvent(event);
    emit focusChanged(false);
    update(); // dim the selection wash now the column is unfocused
}
