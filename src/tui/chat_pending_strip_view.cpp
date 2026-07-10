// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "chat_pending_strip_view.h"

#include "conv_send_controller.h"
#include "pending_ops_model.h"
#include "tui_palette.h"

#include <QRect>
#include <QSize>
#include <Tui/ZColor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>
#include <Tui/ZTerminal.h>

namespace {

Span mkSpan(const QString& text, const Tui::ZColor& fg, const Tui::ZColor& bg,
            Tui::ZTextAttributes attr = {}) {
    return Span{text, fg, bg, attr};
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

mirror::PendingOpsModel* pendingModel(ConvSendController* c) {
    return c != nullptr ? qobject_cast<mirror::PendingOpsModel*>(c->pending()) : nullptr;
}

} // namespace

ChatPendingStripView::ChatPendingStripView(Tui::ZWidget* parent) : Tui::ZWidget(parent) {
    setFocusPolicy(Tui::NoFocus); // gains focus only when it has entries
    setSizePolicyH(Tui::SizePolicy::Expanding);
    setSizePolicyV(Tui::SizePolicy::Fixed);
    setMaximumSize(Tui::tuiMaxSize, 0);
}

int ChatPendingStripView::count() const {
    return m_controller != nullptr ? m_controller->pendingCount() : 0;
}

QString ChatPendingStripView::opIdAt(int row) const {
    mirror::PendingOpsModel* m = pendingModel(m_controller);
    if (m == nullptr || row < 0 || row >= m->rowCount()) {
        return {};
    }
    return m->data(m->index(row, 0), mirror::PendingOpsModel::OpIdRole).toString();
}

int ChatPendingStripView::stateAt(int row) const {
    mirror::PendingOpsModel* m = pendingModel(m_controller);
    if (m == nullptr || row < 0 || row >= m->rowCount()) {
        return -1;
    }
    return m->data(m->index(row, 0), mirror::PendingOpsModel::StateRole).toInt();
}

void ChatPendingStripView::setController(ConvSendController* controller) {
    if (m_controller == controller) {
        return;
    }
    if (m_controller != nullptr) {
        m_controller->disconnect(this);
        if (mirror::PendingOpsModel* m = pendingModel(m_controller)) {
            m->disconnect(this);
        }
    }
    m_controller = controller;
    if (m_controller != nullptr) {
        const auto repaint = [this] { rebuild(); };
        connect(m_controller, &ConvSendController::pendingCountChanged, this, repaint);
        connect(m_controller, &ConvSendController::lanePausedChanged, this, repaint);
        connect(m_controller, &ConvSendController::conversationChanged, this, repaint);
        if (mirror::PendingOpsModel* m = pendingModel(m_controller)) {
            connect(m, &QAbstractItemModel::modelReset, this, repaint);
            connect(m, &QAbstractItemModel::dataChanged, this, repaint);
            connect(m, &QAbstractItemModel::rowsInserted, this, repaint);
            connect(m, &QAbstractItemModel::rowsRemoved, this, repaint);
        }
    }
    rebuild();
}

QSize ChatPendingStripView::sizeHint() const {
    return {40, m_height};
}

void ChatPendingStripView::layoutLines() {
    m_lines.clear();
    const int n = count();
    m_sel = qBound(0, m_sel, qMax(0, n - 1));

    const int w = qMax(1, geometry().width());
    mirror::PendingOpsModel* q = pendingModel(m_controller);
    for (int i = 0; i < n && q != nullptr; ++i) {
        const QModelIndex idx = q->index(i, 0);
        const bool sel = i == m_sel;
        const bool rejected = q->data(idx, mirror::PendingOpsModel::RejectedRole).toBool();
        const int state = q->data(idx, mirror::PendingOpsModel::StateRole).toInt();
        QString text = q->data(idx, mirror::PendingOpsModel::DisplayRole).toString();
        if (rejected) {
            const QString err = q->data(idx, mirror::PendingOpsModel::LastErrorRole).toString();
            if (!err.isEmpty()) {
                text += QStringLiteral(" \u2014 ") + err;
            }
        }

        const Tui::ZColor bg = sel ? tpal::selectionBg() : tpal::bg();
        // Per-state marker (visibly distinct, §6.5): · queued, ↑ sending (inflight), ✓ sent
        // (accepted, awaiting confirm), ✗ rejected. Symbols, not words — no locale surface.
        QString marker = QStringLiteral("\u00b7 "); // Pending
        Tui::ZColor markerFg = sel ? tpal::accent() : tpal::muted();
        if (rejected) {
            marker = QStringLiteral("\u2717 ");
            markerFg = tpal::statusError();
        } else if (state == 1) { // Inflight
            marker = QStringLiteral("\u2191 ");
        } else if (state == 3) { // Accepted
            marker = QStringLiteral("\u2713 ");
        }

        RenderLine line;
        line.push_back(mkSpan(sel ? QStringLiteral("\u25b8 ") : QStringLiteral("  "),
                              sel ? tpal::accent() : tpal::muted(), bg));
        line.push_back(mkSpan(marker, markerFg, bg));
        line.push_back(mkSpan(elide(text, qMax(1, w - 5)),
                              rejected ? tpal::statusError() : tpal::fg(), bg,
                              sel ? Tui::ZTextAttribute::Bold : Tui::ZTextAttributes{}));
        m_lines.push_back(line);
    }
    update();
}

void ChatPendingStripView::rebuild() {
    const int n = count();
    m_height = qMin(n, kMaxRows);
    setFocusPolicy(n > 0 ? Tui::StrongFocus : Tui::NoFocus);
    // Changing our fixed height must re-flow the *parent* column (the QueueStripView lesson):
    // request a parent relayout, then rebuild the lines for the current width.
    setMinimumSize(0, m_height);
    setMaximumSize(Tui::tuiMaxSize, m_height);
    if (Tui::ZWidget* par = parentWidget()) {
        if (Tui::ZTerminal* term = terminal()) {
            term->requestLayout(par);
        }
    }
    layoutLines();
}

void ChatPendingStripView::resizeEvent(Tui::ZResizeEvent* event) {
    Tui::ZWidget::resizeEvent(event);
    layoutLines();
}

void ChatPendingStripView::paintEvent(Tui::ZPaintEvent* event) {
    Tui::ZPainter* p = event->painter();
    p->clear(tpal::fg(), tpal::bg());

    const int h = geometry().height();
    const int w = geometry().width();
    for (int row = 0; row < h && row < static_cast<int>(m_lines.size()); ++row) {
        int x = 0;
        for (const Span& s : m_lines.at(row)) {
            if (x >= w) {
                break;
            }
            QString text = s.text;
            if (x + static_cast<int>(text.size()) > w) {
                text = text.left(w - x);
            }
            if (s.attr != Tui::ZTextAttributes{}) {
                p->writeWithAttributes(x, row, text, s.fg, s.bg, s.attr);
            } else {
                p->writeWithColors(x, row, text, s.fg, s.bg);
            }
            x += static_cast<int>(s.text.size());
        }
    }
}

void ChatPendingStripView::keyEvent(Tui::ZKeyEvent* event) {
    const int n = count();
    if (m_controller != nullptr && n > 0 && event->modifiers() == Qt::NoModifier) {
        const int key = event->key();
        const bool selRejected = stateAt(m_sel) == 2; // OpState::Rejected
        bool handled = true;
        switch (key) {
        case Qt::Key_Up:
            m_sel = qMax(0, m_sel - 1);
            rebuild();
            break;
        case Qt::Key_Down:
            m_sel = qMin(n - 1, m_sel + 1);
            rebuild();
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            m_controller->drain(); // §6.8 manual drain — the user's explicit send tap
            break;
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            m_controller->discard(opIdAt(m_sel));
            break;
        default:
            if (event->text() == QStringLiteral("s")) {
                m_controller->drain();
            } else if (event->text() == QStringLiteral("r") && selRejected) {
                m_controller->retry(opIdAt(m_sel)); // same op-id, dedup-safe (§6.5)
            } else if (event->text() == QStringLiteral("e") && selRejected) {
                emit editRequested(m_controller->takeForEdit(opIdAt(m_sel)));
            } else if (event->text() == QStringLiteral("d")) {
                m_controller->discard(opIdAt(m_sel));
            } else if (event->text() == QStringLiteral("a")) {
                m_controller->sendRemainingAnyway(); // paused-lane override (§6.5)
            } else {
                handled = false;
            }
            break;
        }
        if (handled) {
            event->accept();
            return;
        }
    }
    Tui::ZWidget::keyEvent(event);
}
