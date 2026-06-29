// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "tab_strip_view.h"

#include "tab_model.h"
#include "tui_palette.h"

#include <algorithm>
#include <QRect>
#include <QSize>
#include <Tui/ZColor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

namespace {

constexpr int kMaxTitle = 16; // cap a chip's title width

// The on-screen width of a chip for `title` (closable adds an "x" column):
//   " " + title(capped) + " " + ["x"] + " "
int chipWidth(const QString& title, bool closable) {
    const int len = qMin(static_cast<int>(title.size()), kMaxTitle);
    return 1 + len + 1 + (closable ? 1 : 0) + 1;
}

QString elide(const QString& text, int width) {
    if (width <= 0) {
        return {};
    }
    if (text.size() <= width) {
        return text;
    }
    return text.left(width - 1) + QStringLiteral("\u2026");
}

} // namespace

TabStripView::TabStripView(Tui::ZWidget* parent) : Tui::ZWidget(parent) {
    setSizePolicyH(Tui::SizePolicy::Expanding);
    setSizePolicyV(Tui::SizePolicy::Fixed);
    setMaximumSize(Tui::tuiMaxSize, 1);
    setFocusPolicy(Tui::StrongFocus); // a Tab-cycle stop; Left/Right move tabs
}

void TabStripView::setModel(TabModel* model) {
    m_model = model;
    if (m_model != nullptr) {
        const auto repaint = [this] {
            layout();
            update();
        };
        connect(m_model, &QAbstractItemModel::modelReset, this, repaint);
        connect(m_model, &QAbstractItemModel::dataChanged, this, repaint);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, repaint);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, repaint);
        connect(m_model, &TabModel::currentIndexChanged, this, repaint);
    }
    layout();
    update();
}

QSize TabStripView::sizeHint() const {
    return {40, 1};
}

void TabStripView::layout() {
    m_segments.clear();
    if (m_model == nullptr) {
        return;
    }
    const int n = m_model->count();
    const int w = qMax(1, geometry().width());
    const int active = m_model->currentIndex();

    // Keep the active tab visible: never start past it, and advance the scroll
    // anchor until the active chip's right edge fits within the width.
    if (active >= 0 && active < m_firstVisible) {
        m_firstVisible = active;
    }
    m_firstVisible = std::min(m_firstVisible, qMax(0, n - 1));

    const auto build = [&](int first) {
        QVector<Segment> segs;
        int x = 0;
        for (int i = first; i < n; ++i) {
            const QString title = m_model->titleAt(i);
            const bool closable =
                m_model->data(m_model->index(i, 0), TabModel::ClosableRole).toBool();
            const int cw = chipWidth(title, closable);
            Segment seg;
            seg.index = i;
            seg.x0 = x;
            seg.x1 = x + cw;
            seg.closeX = closable ? (seg.x1 - 2) : -1;
            segs.push_back(seg);
            x += cw;
            if (x >= w) {
                break;
            }
        }
        return segs;
    };

    if (active >= 0) {
        // Advance the anchor until the active chip fits (or we cannot scroll more).
        while (m_firstVisible < active) {
            const QVector<Segment> segs = build(m_firstVisible);
            bool fits = false;
            for (const Segment& s : segs) {
                if (s.index == active && s.x1 <= w) {
                    fits = true;
                    break;
                }
            }
            if (fits) {
                break;
            }
            ++m_firstVisible;
        }
    }

    m_segments = build(m_firstVisible);

    // Append the trailing "+" affordance when it fits.
    int end = m_segments.isEmpty() ? 0 : m_segments.last().x1;
    if (end + 3 <= w) {
        Segment plus;
        plus.plus = true;
        plus.x0 = end;
        plus.x1 = end + 3; // " + "
        m_segments.push_back(plus);
    }
}

void TabStripView::resizeEvent(Tui::ZResizeEvent* event) {
    Tui::ZWidget::resizeEvent(event);
    layout();
}

void TabStripView::clickAt(QPoint local) {
    if (m_model == nullptr) {
        return;
    }
    layout();
    const int x = local.x();
    for (const Segment& s : m_segments) {
        if (x < s.x0 || x >= s.x1) {
            continue;
        }
        if (s.plus) {
            emit newTabRequested();
            return;
        }
        if (s.closeX >= 0 && x == s.closeX) {
            m_model->closeTab(s.index);
        } else {
            m_model->activate(s.index);
        }
        return;
    }
}

void TabStripView::keyEvent(Tui::ZKeyEvent* event) {
    if (m_model != nullptr && event->modifiers() == Qt::NoModifier) {
        const int n = m_model->count();
        const int cur = m_model->currentIndex();
        const int key = event->key();
        bool handled = true;
        if (key == Qt::Key_Left) {
            // Arrows move and stop at the ends; global Ctrl+Tab keeps wrapping.
            m_model->activate(qMax(0, cur - 1));
        } else if (key == Qt::Key_Right) {
            m_model->activate(qMin(n - 1, cur + 1));
        } else if (key == Qt::Key_Enter || key == Qt::Key_Return) {
            m_model->pinCurrent(); // graduate a preview tab, like the GUI double-click
        } else if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
            if (cur >= 0 &&
                m_model->data(m_model->index(cur, 0), TabModel::ClosableRole).toBool()) {
                m_model->closeTab(cur);
            }
        } else {
            handled = false; // leave plain Tab et al. for the focus container
        }
        if (handled) {
            event->accept();
            return;
        }
    }
    Tui::ZWidget::keyEvent(event);
}

void TabStripView::focusInEvent(Tui::ZFocusEvent* event) {
    Tui::ZWidget::focusInEvent(event);
    update();
}

void TabStripView::focusOutEvent(Tui::ZFocusEvent* event) {
    Tui::ZWidget::focusOutEvent(event);
    update();
}

void TabStripView::paintEvent(Tui::ZPaintEvent* event) {
    Tui::ZPainter* p = event->painter();
    p->clear(tpal::muted(), tpal::surfaceAlt());
    if (m_model == nullptr) {
        return;
    }
    layout();

    const int w = geometry().width();
    const int active = m_model->currentIndex();
    const bool focused = focus();

    for (const Segment& s : m_segments) {
        if (s.x0 >= w) {
            break;
        }
        if (s.plus) {
            p->writeWithColors(s.x0, 0, QStringLiteral(" + "), tpal::accent(), tpal::surfaceAlt());
            continue;
        }
        const bool isActive = s.index == active;
        const bool isPreview = m_model->isPreviewAt(s.index);
        // Brighter wash for the active chip when the strip holds focus, weaker
        // when another pane is focused (mirrors the session list).
        const Tui::ZColor activeBg = focused ? tpal::selectionBg() : tpal::selectionInactiveBg();
        const Tui::ZColor bg = isActive ? activeBg : tpal::surfaceAlt();
        const Tui::ZColor fg = isActive ? tpal::fg() : tpal::muted();
        const bool closable = s.closeX >= 0;
        const QString title = elide(m_model->titleAt(s.index), kMaxTitle);

        // Paint the chip background across its full extent first.
        for (int cx = s.x0; cx < s.x1 && cx < w; ++cx) {
            p->writeWithColors(cx, 0, QStringLiteral(" "), fg, bg);
        }
        int tx = s.x0 + 1;
        if (tx < w) {
            const QString shown = title.left(qMax(0, w - tx));
            // A preview (transient) tab is shown italic; the active tab is bold.
            // Both apply when the active tab is itself a preview.
            Tui::ZTextAttributes attrs;
            if (isActive) {
                attrs |= Tui::ZTextAttribute::Bold;
            }
            if (isPreview) {
                attrs |= Tui::ZTextAttribute::Italic;
            }
            if (attrs != Tui::ZTextAttributes()) {
                p->writeWithAttributes(tx, 0, shown, fg, bg, attrs);
            } else {
                p->writeWithColors(tx, 0, shown, fg, bg);
            }
        }
        if (closable && s.closeX >= 0 && s.closeX < w) {
            // A File tab with unsaved changes shows a dirty dot in place of the
            // close glyph (VS Code-style); other tabs show the close affordance.
            const bool dirty =
                m_model->data(m_model->index(s.index, 0), TabModel::DirtyRole).toBool();
            if (dirty) {
                p->writeWithColors(s.closeX, 0, QStringLiteral("\u25cf"), tpal::accent(), bg);
            } else {
                p->writeWithColors(s.closeX, 0, QStringLiteral("\u00d7"),
                                   isActive ? tpal::warn() : tpal::muted(), bg);
            }
        }
    }
}
