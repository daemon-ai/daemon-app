// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>
#include <QVector>
#include <Tui/ZWidget.h>

class CompletionModel;

// Custom-painted completion popup replacing the stock Tui::ZListView overlay,
// mirroring the GUI's CompletionPopover.qml. Each item row shows a leading kind
// badge ("/" for slash, "@" for mention), a bold label and a muted hint; the
// active row gets a selection wash and group changes insert a faint header. The
// shell (RootWidget) keeps owning the float geometry/visibility and routes
// navigation through ComposerSessionController; this view only renders.
class CompletionView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit CompletionView(Tui::ZWidget* parent = nullptr);

    // Rebuild from the model + active item + trigger kind ("slash"/"mention").
    void setData(CompletionModel* model, int activeIndex, const QString& kind);

    // Number of rendered lines (item rows + group headers), for the shell to size
    // the float.
    [[nodiscard]] int desiredHeight() const { return static_cast<int>(m_rows.size()); }

    // Mouse: the completion model row rendered at widget-local y, or -1 when that
    // line is a group header or empty. Accounts for the scroll the paint applies.
    [[nodiscard]] int modelRowAt(int localY) const;

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;

private:
    struct Row {
        bool header = false;
        int modelRow = -1;
        QString group;
        QString label;
        QString hint;
    };

    [[nodiscard]] int topForActive(int h) const;

    QVector<Row> m_rows;
    int m_activeIndex = 0;
    QString m_kind;
};
