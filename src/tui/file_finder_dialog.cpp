// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "file_finder_dialog.h"

#include "file_finder_model.h"

#include <algorithm>
#include <QAbstractItemModel>
#include <QRect>
#include <QSize>
#include <QStringList>
#include <Tui/ZVBoxLayout.h>
#include <utility>

FileFinderDialog::FileFinderDialog(const QString& title, QString hint,
                                   files::FileFinderModel* model, Tui::ZWidget* parent)
    : Tui::ZDialog(parent), m_model(model), m_hint(std::move(hint)) {
    setWindowTitle(title);
    setContentsMargins({1, 1, 1, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    m_filter = new PaletteInput(this);
    layout->addWidget(m_filter);

    m_list = new Tui::ZListView(this);
    layout->addWidget(m_list);

    m_status = new Tui::ZLabel(m_hint, this);
    layout->addWidget(m_status);

    // The filter text IS the model query: the model reranks (a model reset)
    // and rebuild() refreshes the rows. Async index batches land the same way.
    connect(m_filter, &Tui::ZInputBox::textChanged, this,
            [this](const QString& text) { m_model->setQuery(text); });
    connect(m_model, &QAbstractItemModel::modelReset, this, [this] { rebuild(); });
    connect(m_model, &files::FileFinderModel::indexingChanged, this, [this] { updateStatus(); });

    connect(m_filter, &PaletteInput::moveUp, this, [this] { step(-1); });
    connect(m_filter, &PaletteInput::moveDown, this, [this] { step(1); });
    connect(m_filter, &PaletteInput::accepted, this,
            [this] { activateRow(m_list->currentIndex().row(), false); });
    connect(m_filter, &PaletteInput::acceptedAlt, this,
            [this] { activateRow(m_list->currentIndex().row(), true); });
    connect(m_filter, &PaletteInput::canceled, this, [this] {
        setVisible(false);
        emit dismissed();
    });
    // Mouse / Enter on the list itself opens the default (preview) way.
    connect(m_list, &Tui::ZListView::enterPressed, this,
            [this](int selected) { activateRow(selected, false); });
}

void FileFinderDialog::openCentered() {
    // Size to a comfortable fraction of the parent, then center (a little wider
    // than the command palette so workspace paths fit).
    Tui::ZWidget* p = parentWidget();
    const QSize parentSize = p != nullptr ? p->geometry().size() : QSize(80, 24);
    const int w = qBound(32, parentSize.width() - 8, 72);
    const int h = qBound(9, parentSize.height() - 6, 19);
    const int x = qMax(0, (parentSize.width() - w) / 2);
    const int y = qMax(0, (parentSize.height() - h) / 2);
    setGeometry(QRect(x, y, w, h));

    // Self-heal an empty index (e.g. the fs seam had no roots yet when the
    // model was wired at startup): re-request the roots, which restarts the
    // walk. A populated index is NOT re-walked - same lifetime as the GUI's
    // finder, which indexes once per rootsChanged.
    if (m_model->fileCount() == 0 && !m_model->indexing()) {
        m_model->rebuildIndex();
    }
    if (m_filter != nullptr) {
        m_filter->setText(QString()); // -> setQuery("") -> rerank when needed
    }
    m_model->setQuery(QString());
    rebuild();
    updateStatus();
    setVisible(true);
    raise();
    if (m_filter != nullptr) {
        m_filter->setFocus();
    }
}

void FileFinderDialog::rebuild() {
    QStringList display;
    const int rows = m_model->rowCount();
    display.reserve(rows);
    for (int i = 0; i < rows; ++i) {
        const QModelIndex idx = m_model->index(i, 0);
        const QString name = m_model->data(idx, files::FileFinderModel::NameRole).toString();
        const QString path = m_model->data(idx, files::FileFinderModel::PathRole).toString();
        // Name first (what the GUI emphasizes), the disambiguating relative
        // path after it - except for root-level files where they coincide.
        QString line = name;
        if (path != name) {
            line += QStringLiteral("   ") + path;
        }
        display << line;
    }
    m_list->setItems(display);
    if (!display.isEmpty() && m_list->model() != nullptr) {
        m_list->setCurrentIndex(m_list->model()->index(0, 0));
    }
}

void FileFinderDialog::step(int delta) {
    if (m_list->model() == nullptr || m_model->rowCount() == 0) {
        return;
    }
    int row = m_list->currentIndex().row();
    row = std::max(row, 0);
    row = qBound(0, row + delta, m_model->rowCount() - 1);
    m_list->setCurrentIndex(m_list->model()->index(row, 0));
}

void FileFinderDialog::activateRow(int row, bool pinned) {
    if (row < 0 || row >= m_model->rowCount()) {
        return;
    }
    const QModelIndex idx = m_model->index(row, 0);
    const QString rootId = m_model->data(idx, files::FileFinderModel::RootIdRole).toString();
    const QString path = m_model->data(idx, files::FileFinderModel::PathRole).toString();
    setVisible(false);
    // Rank the choice first on the next open (FileFinder.qml activateCurrent
    // parity). Reranks the (now hidden) list; read the row before this.
    m_model->noteRecent(rootId, path);
    emit chosen(rootId, path, pinned);
}

void FileFinderDialog::updateStatus() {
    if (m_status == nullptr) {
        return;
    }
    m_status->setText(
        m_model->indexing() ? tr("Indexing\u2026 (%n file(s))", "", m_model->fileCount()) : m_hint);
}
