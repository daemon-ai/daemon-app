#pragma once

#include "transcript_render.h" // Span / RenderLine

#include <Tui/ZWidget.h>

#include <QTimer>
#include <QVector>

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
};
