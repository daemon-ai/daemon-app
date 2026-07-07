// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transcript_render.h" // Span / RenderLine

#include <QTimer>
#include <QVector>
#include <Tui/ZWidget.h>

class StatusBarModel;

// Custom-painted footer status strip replacing the plain Tui::ZLabel. Renders the
// shared StatusBarModel as colored segments mirroring the GUI's StatusBar.qml:
// a left cluster (Gateway health, Agents) and a right cluster (Running spinner +
// turn timer, Context label + gauge, Session timer, app version). A short timer
// advances the spinner frame only while a turn is in flight.
class StatusBarView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit StatusBarView(Tui::ZWidget* parent = nullptr);

    void setModel(StatusBarModel* model);
    // TOOL-8: the pending-approvals count (fed from the shared IApprovalsInbox in root wiring). A
    // non-zero count paints an accented "gate" segment in the footer — the TUI parity of the GUI
    // sidebar badge. Repaints on change.
    void setPendingApprovals(int count);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;

private:
    [[nodiscard]] QVector<Span> buildLeft() const;
    [[nodiscard]] QVector<Span> buildRight() const;
    void syncSpinner();

    StatusBarModel* m_model = nullptr;
    QTimer m_spinner;
    int m_spinnerTick = 0;
    int m_pendingApprovals = 0; // TOOL-8 footer gate segment
};
