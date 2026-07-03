// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "palette_dialog.h"

#include <QString>
#include <Tui/ZDialog.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>

namespace files {
class FileFinderModel;
}

// A centered fuzzy file-finder overlay over the shared files::FileFinderModel
// (the same view-model behind the GUI's FileFinder.qml). Unlike PaletteDialog,
// whose rows are a static snapshot, this list is model-driven: the filter text
// feeds the model's query (fuzzy rerank over the async workspace index) and the
// rows rebuild on every model reset, so results stream in while the index walk
// is still running. Enter chooses the selection (pinned=false), Shift+Enter is
// the deliberate/pinned variant, Esc dismisses. Backs both the Ctrl+G
// "go to file" overlay and the composer's Ctrl+O attachment picker.
class FileFinderDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    // `hint` is the resting status-line text (shown when not indexing).
    FileFinderDialog(const QString& title, QString hint, files::FileFinderModel* model,
                     Tui::ZWidget* parent);

    // Show centered in the parent, clear the query, and focus the filter.
    void openCentered();

signals:
    // A file was chosen: Enter (pinned=false) or Shift+Enter (pinned=true).
    void chosen(const QString& rootId, const QString& path, bool pinned);
    // The dialog was closed without a choice (Esc).
    void dismissed();

private:
    void rebuild();       // model rows -> visible list rows (selection to top)
    void step(int delta); // move the list selection
    void activateRow(int row, bool pinned);
    void updateStatus(); // "Indexing… (n files)" while the walk runs, else hint

    files::FileFinderModel* m_model = nullptr;
    QString m_hint;
    PaletteInput* m_filter = nullptr;
    Tui::ZListView* m_list = nullptr;
    Tui::ZLabel* m_status = nullptr;
};
