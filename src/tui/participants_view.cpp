#include "participants_view.h"

#include "tui_palette.h"

#include "participants_model.h"

#include <Tui/ZColor.h>
#include <Tui/ZCommon.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

#include <QModelIndex>
#include <QRect>

using participants::ParticipantsModel;

ParticipantsView::ParticipantsView(Tui::ZWidget* parent) : Tui::ZWidget(parent)
{
    setFocusPolicy(Tui::NoFocus);
    setSizePolicyV(Tui::SizePolicy::Fixed);
}

void ParticipantsView::setModel(ParticipantsModel* model)
{
    m_model = model;
    if (m_model != nullptr) {
        const auto refreshSlot = [this] { refresh(); };
        connect(m_model, &QAbstractItemModel::modelReset, this, refreshSlot);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, refreshSlot);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, refreshSlot);
        connect(m_model, &QAbstractItemModel::dataChanged, this, [this] { update(); });
    }
    refresh();
}

void ParticipantsView::refresh()
{
    // Fixed height = the number of rows (header + participants), so the Explorer
    // stacked below gets all the remaining vertical space.
    const int rows = m_model != nullptr ? m_model->rowCount() : 0;
    const int h = qMax(1, rows);
    setMinimumSize(0, h);
    setMaximumSize(Tui::tuiMaxSize, h);
    update();
}

void ParticipantsView::paintEvent(Tui::ZPaintEvent* event)
{
    Tui::ZPainter* p = event->painter();
    const Tui::ZColor fg = tpal::fg();
    const Tui::ZColor bg = tpal::bg();
    p->clear(fg, bg);

    const int h = geometry().height();
    const int w = geometry().width();

    for (int rowY = 0; rowY < h; ++rowY) {
        if (m_model == nullptr || rowY >= m_model->rowCount()) {
            continue;
        }
        const QModelIndex idx = m_model->index(rowY, 0);
        const QString label = m_model->data(idx, ParticipantsModel::LabelRole).toString();

        if (m_model->data(idx, ParticipantsModel::IsSeparatorRole).toBool()) {
            const bool expanded = m_model->data(idx, ParticipantsModel::ExpandedRole).toBool();
            const QString tw
                = expanded ? QStringLiteral("\u25be ") : QStringLiteral("\u25b8 ");
            const QString text
                = QStringLiteral("%1\u2500\u2500 %2 \u2500\u2500").arg(tw, label);
            p->writeWithColors(0, rowY, text.left(qMax(0, w)), tpal::muted(), bg);
            continue;
        }

        // Participant row: "  <dot> <name>" with a green dot for "available".
        const QString presence = m_model->data(idx, ParticipantsModel::PresenceRole).toString();
        const Tui::ZColor dotFg
            = presence == QStringLiteral("available") ? tpal::statusOk() : tpal::muted();
        if (w > 2) {
            p->writeWithColors(2, rowY, QStringLiteral("\u25cf"), dotFg, bg);
        }
        if (w > 4) {
            p->writeWithColors(4, rowY, label.left(w - 4), fg, bg);
        }
    }
}
