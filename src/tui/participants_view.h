#pragma once

#include <Tui/ZWidget.h>

namespace participants {
class ParticipantsModel;
}

// Custom-painted "Participants" section for the right column, over the shared
// ParticipantsModel (the same model the GUI's Participants.qml renders). Paints a
// section header followed by one row per participant, each with a colored presence
// dot (green for "available"). Display-only and fixed-height (= row count) so the
// file Explorer below gets the remaining space.
class ParticipantsView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit ParticipantsView(Tui::ZWidget* parent = nullptr);

    void setModel(participants::ParticipantsModel* model);

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;

private:
    void refresh();

    participants::ParticipantsModel* m_model = nullptr;
};
