#include "completion_view.h"

#include "tui_palette.h"

#include "completion_model.h"

#include <Tui/ZColor.h>
#include <Tui/ZCommon.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

#include <QRect>

CompletionView::CompletionView(Tui::ZWidget* parent) : Tui::ZWidget(parent)
{
    setVisible(false);
}

void CompletionView::setData(CompletionModel* model, int activeIndex, const QString& kind)
{
    m_rows.clear();
    m_kind = kind;
    if (model == nullptr) {
        m_activeIndex = 0;
        update();
        return;
    }

    const int n = model->count();
    m_activeIndex = qBound(0, activeIndex, qMax(0, n - 1));

    QString lastGroup;
    for (int i = 0; i < n; ++i) {
        const QModelIndex idx = model->index(i, 0);
        const QString group = model->data(idx, CompletionModel::GroupRole).toString();
        if (!group.isEmpty() && group != lastGroup) {
            Row header;
            header.header = true;
            header.group = group;
            m_rows.push_back(header);
            lastGroup = group;
        }
        Row r;
        r.modelRow = i;
        r.label = model->data(idx, CompletionModel::LabelRole).toString();
        r.hint = model->data(idx, CompletionModel::HintRole).toString();
        m_rows.push_back(r);
    }
    update();
}

int CompletionView::modelRowAt(int localY) const
{
    if (localY < 0 || localY >= geometry().height()) {
        return -1;
    }
    const int idx = topForActive(geometry().height()) + localY;
    if (idx < 0 || idx >= static_cast<int>(m_rows.size())) {
        return -1;
    }
    const Row& r = m_rows.at(idx);
    return r.header ? -1 : r.modelRow;
}

int CompletionView::topForActive(int h) const
{
    // Find the rendered line of the active item and scroll so it stays visible.
    int activeLine = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).modelRow == m_activeIndex) {
            activeLine = i;
            break;
        }
    }
    const int total = static_cast<int>(m_rows.size());
    if (total <= h) {
        return 0;
    }
    int top = activeLine - h / 2;
    top = qBound(0, top, total - h);
    return top;
}

void CompletionView::paintEvent(Tui::ZPaintEvent* event)
{
    Tui::ZPainter* p = event->painter();
    p->clear(tpal::fg(), tpal::surfaceAlt());

    const int w = geometry().width();
    const int h = geometry().height();
    const int top = topForActive(h);

    for (int rowY = 0; rowY < h; ++rowY) {
        const int idx = top + rowY;
        if (idx < 0 || idx >= static_cast<int>(m_rows.size())) {
            continue;
        }
        const Row& r = m_rows.at(idx);

        if (r.header) {
            const QString text = QStringLiteral("\u2500 ") + r.group + QStringLiteral(" ");
            p->writeWithColors(0, rowY, text.left(w), tpal::faint(), tpal::surfaceAlt());
            continue;
        }

        const bool active = r.modelRow == m_activeIndex;
        const Tui::ZColor rowBg = active ? tpal::selectionBg() : tpal::surfaceAlt();
        if (active) {
            p->clearRect(0, rowY, w, 1, tpal::fg(), rowBg);
        }

        int x = 0;
        // Kind badge (accent on a recessed cell, like the GUI's leading chip).
        const QString badge = (m_kind == QStringLiteral("mention")) ? QStringLiteral(" @ ")
                                                                     : QStringLiteral(" / ");
        p->writeWithColors(x, rowY, badge, tpal::accent(), active ? rowBg : tpal::codeBg());
        x += static_cast<int>(badge.size());

        // Label (bold).
        if (x < w && !r.label.isEmpty()) {
            QString label = QStringLiteral(" ") + r.label;
            if (x + static_cast<int>(label.size()) > w) {
                label = label.left(w - x);
            }
            p->writeWithAttributes(x, rowY, label, tpal::fg(), rowBg, Tui::ZTextAttribute::Bold);
            x += static_cast<int>(label.size());
        }

        // Hint (muted), trailing.
        if (x < w && !r.hint.isEmpty()) {
            QString hint = QStringLiteral("  ") + r.hint;
            if (x + static_cast<int>(hint.size()) > w) {
                hint = hint.left(w - x);
            }
            p->writeWithColors(x, rowY, hint, tpal::muted(), rowBg);
        }
    }
}
