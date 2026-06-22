#include "attachment_bar_view.h"

#include "tui_palette.h"

#include "composer_attachment_model.h"
#include "composer_session_controller.h"

#include <Tui/ZColor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>
#include <Tui/ZTerminal.h>

#include <QRect>
#include <QSize>

namespace {

Span mkSpan(const QString& text, const Tui::ZColor& fg, const Tui::ZColor& bg)
{
    return Span { text, fg, bg, {} };
}

QString kindGlyph(const QString& kind)
{
    if (kind == QStringLiteral("image")) {
        return QStringLiteral("\u25a3"); // ▣
    }
    if (kind == QStringLiteral("folder")) {
        return QStringLiteral("\u25b1"); // ▱
    }
    if (kind == QStringLiteral("url")) {
        return QStringLiteral("\u29c9"); // ⧉
    }
    return QStringLiteral("\u25a4"); // ▤ file
}

} // namespace

AttachmentBarView::AttachmentBarView(Tui::ZWidget* parent) : Tui::ZWidget(parent)
{
    setFocusPolicy(Tui::NoFocus);
    setSizePolicyH(Tui::SizePolicy::Expanding);
    setSizePolicyV(Tui::SizePolicy::Fixed);
    setMaximumSize(Tui::tuiMaxSize, 0);
}

int AttachmentBarView::count() const
{
    return m_controller != nullptr ? m_controller->attachments()->count() : 0;
}

void AttachmentBarView::setController(ComposerSessionController* controller)
{
    m_controller = controller;
    if (m_controller != nullptr) {
        if (ComposerAttachmentModel* a = m_controller->attachments()) {
            const auto repaint = [this] { rebuild(); };
            connect(a, &ComposerAttachmentModel::countChanged, this, repaint);
            connect(a, &QAbstractItemModel::modelReset, this, repaint);
            connect(a, &QAbstractItemModel::rowsInserted, this, repaint);
            connect(a, &QAbstractItemModel::rowsRemoved, this, repaint);
        }
    }
    rebuild();
}

QSize AttachmentBarView::sizeHint() const
{
    return { 40, m_height };
}

void AttachmentBarView::rebuild()
{
    m_line.clear();
    const int n = count();
    m_sel = qBound(0, m_sel, qMax(0, n - 1));

    ComposerAttachmentModel* a = m_controller != nullptr ? m_controller->attachments() : nullptr;
    for (int i = 0; i < n && a != nullptr; ++i) {
        const QModelIndex idx = a->index(i, 0);
        const QString name = a->data(idx, ComposerAttachmentModel::NameRole).toString();
        const QString kind = a->data(idx, ComposerAttachmentModel::KindRole).toString();
        const bool sel = i == m_sel;
        const Tui::ZColor bg = sel ? tpal::selectionBg() : tpal::codeBg();
        m_line.push_back(mkSpan(QStringLiteral(" ") + kindGlyph(kind) + QStringLiteral(" "),
                                tpal::accent(), bg));
        m_line.push_back(mkSpan(name + QStringLiteral(" "), tpal::fg(), bg));
        if (sel) {
            m_line.push_back(mkSpan(QStringLiteral("\u2715 "), tpal::statusError(), bg));
        }
        m_line.push_back(mkSpan(QStringLiteral("  "), tpal::fg(), tpal::bg()));
    }

    m_height = n > 0 ? 1 : 0;
    setFocusPolicy(n > 0 ? Tui::StrongFocus : Tui::NoFocus);
    setMinimumSize(0, m_height);
    setMaximumSize(Tui::tuiMaxSize, m_height);
    // Re-flow the parent column so our new fixed height actually takes effect.
    if (Tui::ZWidget* par = parentWidget()) {
        if (Tui::ZTerminal* term = terminal()) {
            term->requestLayout(par);
        }
    }
    update();
}

void AttachmentBarView::clickAt(QPoint local)
{
    const int n = count();
    if (n <= 0 || local.y() != 0) {
        return;
    }
    ComposerAttachmentModel* a = m_controller != nullptr ? m_controller->attachments() : nullptr;
    if (a == nullptr) {
        return;
    }
    // Walk the chips left-to-right, reproducing rebuild()'s span widths (the
    // selected chip carries an extra "x " marker), and pick the one the x falls in.
    int x = 0;
    for (int i = 0; i < n; ++i) {
        const QString name = a->data(a->index(i, 0), ComposerAttachmentModel::NameRole).toString();
        const int wGlyph = 3;               // " <glyph> "
        const int wName = static_cast<int>(name.size()) + 1; // name + trailing space
        const int wClose = (i == m_sel) ? 2 : 0; // "x " on the selected chip
        const int wTrail = 2;               // gap between chips
        const int start = x;
        const int end = start + wGlyph + wName + wClose;
        if (local.x() >= start && local.x() < end) {
            if (i != m_sel) {
                m_sel = i;
                rebuild();
            }
            return;
        }
        x = end + wTrail;
    }
}

void AttachmentBarView::paintEvent(Tui::ZPaintEvent* event)
{
    Tui::ZPainter* p = event->painter();
    p->clear(tpal::fg(), tpal::bg());

    const int w = geometry().width();
    int x = 0;
    for (const Span& s : m_line) {
        if (x >= w) {
            break;
        }
        QString text = s.text;
        if (x + static_cast<int>(text.size()) > w) {
            text = text.left(w - x);
        }
        p->writeWithColors(x, 0, text, s.fg, s.bg);
        x += static_cast<int>(s.text.size());
    }
}

void AttachmentBarView::keyEvent(Tui::ZKeyEvent* event)
{
    const int n = count();
    if (m_controller != nullptr && n > 0 && event->modifiers() == Qt::NoModifier) {
        const int key = event->key();
        bool handled = true;
        if (key == Qt::Key_Left) {
            m_sel = qMax(0, m_sel - 1);
            rebuild();
        } else if (key == Qt::Key_Right) {
            m_sel = qMin(n - 1, m_sel + 1);
            rebuild();
        } else if (key == Qt::Key_Delete || key == Qt::Key_Backspace
                   || event->text() == QStringLiteral("x")) {
            m_controller->attachments()->removeAt(m_sel);
        } else {
            handled = false;
        }
        if (handled) {
            event->accept();
            return;
        }
    }
    Tui::ZWidget::keyEvent(event);
}
