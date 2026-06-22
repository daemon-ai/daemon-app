#pragma once

#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZListView.h>

#include <QString>
#include <QVector>

// A one-line filter input that drives a sibling result list: Up/Down move the
// list selection, Enter activates, Esc cancels; everything else edits the text.
// (The list never takes focus, so typing always lands in the filter.)
class PaletteInput : public Tui::ZInputBox {
    Q_OBJECT

public:
    using Tui::ZInputBox::ZInputBox;

signals:
    void moveUp();
    void moveDown();
    void accepted();
    void canceled();

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;
};

// A reusable, centered, filterable overlay: a filter field above a scrollable
// result list. Backs both the TUI model picker and the command palette. The owner
// supplies entries {id,title,hint}; activation (Enter / list enterPressed) emits
// activated(id) and hides the dialog. Esc hides without activating.
class PaletteDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    struct Item {
        QString id;
        QString title;
        QString hint;
    };

    explicit PaletteDialog(const QString& title, Tui::ZWidget* parent);

    // Replace the entry set and reset the filter; call before opening.
    void setItems(const QVector<Item>& items);
    // Show centered in the parent, clear the filter, and focus it.
    void openCentered();

signals:
    void activated(const QString& id);

private:
    void rebuild();        // apply the filter text -> visible list rows
    void step(int delta);  // move the list selection within the filtered rows
    void activateRow(int row);

    QVector<Item> m_items;
    QVector<int> m_filtered; // list row -> index into m_items
    PaletteInput* m_filter = nullptr;
    Tui::ZListView* m_list = nullptr;
};
