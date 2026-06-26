#include "palette_dialog.h"

#include <algorithm>
#include <QAbstractItemModel>
#include <QRect>
#include <Tui/ZEvent.h>
#include <Tui/ZLabel.h>
#include <Tui/ZVBoxLayout.h>

void PaletteInput::keyEvent(Tui::ZKeyEvent* event) {
    const int key = event->key();
    if (key == Qt::Key_Up) {
        emit moveUp();
        event->accept();
        return;
    }
    if (key == Qt::Key_Down) {
        emit moveDown();
        event->accept();
        return;
    }
    if (key == Qt::Key_Enter || key == Qt::Key_Return || event->text() == QStringLiteral("\r")) {
        emit accepted();
        event->accept();
        return;
    }
    if (key == Qt::Key_Escape) {
        emit canceled();
        event->accept();
        return;
    }
    Tui::ZInputBox::keyEvent(event);
}

PaletteDialog::PaletteDialog(const QString& title, Tui::ZWidget* parent) : Tui::ZDialog(parent) {
    setWindowTitle(title);
    setContentsMargins({1, 1, 1, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    m_filter = new PaletteInput(this);
    layout->addWidget(m_filter);

    m_list = new Tui::ZListView(this);
    layout->addWidget(m_list);

    connect(m_filter, &Tui::ZInputBox::textChanged, this, [this] { rebuild(); });
    connect(m_filter, &PaletteInput::moveUp, this, [this] { step(-1); });
    connect(m_filter, &PaletteInput::moveDown, this, [this] { step(1); });
    connect(m_filter, &PaletteInput::accepted, this,
            [this] { activateRow(m_list->currentIndex().row()); });
    connect(m_filter, &PaletteInput::canceled, this, [this] { setVisible(false); });
    // Mouse / Enter on the list itself: `selected` is the visible row.
    connect(m_list, &Tui::ZListView::enterPressed, this,
            [this](int selected) { activateRow(selected); });
}

void PaletteDialog::setItems(const QVector<Item>& items) {
    m_items = items;
    if (m_filter != nullptr) {
        m_filter->setText(QString());
    }
    rebuild();
}

void PaletteDialog::openCentered() {
    // Size to a comfortable fraction of the parent, then center.
    Tui::ZWidget* p = parentWidget();
    const QSize parentSize = p != nullptr ? p->geometry().size() : QSize(80, 24);
    const int w = qBound(28, parentSize.width() - 8, 64);
    const int h = qBound(8, parentSize.height() - 6, 18);
    const int x = qMax(0, (parentSize.width() - w) / 2);
    const int y = qMax(0, (parentSize.height() - h) / 2);
    setGeometry(QRect(x, y, w, h));

    if (m_filter != nullptr) {
        m_filter->setText(QString());
    }
    rebuild();
    setVisible(true);
    raise();
    if (m_filter != nullptr) {
        m_filter->setFocus();
    }
}

void PaletteDialog::rebuild() {
    const QString q = m_filter != nullptr ? m_filter->text().toLower() : QString();
    QStringList display;
    m_filtered.clear();
    for (int i = 0; i < m_items.size(); ++i) {
        const Item& it = m_items.at(i);
        if (!q.isEmpty()) {
            const QString hay =
                (it.title + QStringLiteral(" ") + it.hint + QStringLiteral(" ") + it.id).toLower();
            if (!hay.contains(q)) {
                continue;
            }
        }
        m_filtered.append(i);
        QString line = it.title;
        if (!it.hint.isEmpty()) {
            line += QStringLiteral("   ") + it.hint;
        }
        display << line;
    }
    m_list->setItems(display);
    if (!display.isEmpty() && m_list->model() != nullptr) {
        m_list->setCurrentIndex(m_list->model()->index(0, 0));
    }
}

void PaletteDialog::step(int delta) {
    if (m_filtered.isEmpty() || m_list->model() == nullptr) {
        return;
    }
    int row = m_list->currentIndex().row();
    row = std::max(row, 0);
    row = qBound(0, row + delta, static_cast<int>(m_filtered.size()) - 1);
    m_list->setCurrentIndex(m_list->model()->index(row, 0));
}

void PaletteDialog::activateRow(int row) {
    if (row < 0 || row >= m_filtered.size()) {
        return;
    }
    const QString id = m_items.at(m_filtered.at(row)).id;
    setVisible(false);
    emit activated(id);
}
