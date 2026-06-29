// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transcript_render.h" // Span / RenderLine

#include <QVector>
#include <Tui/ZWidget.h>

class ComposerSessionController;

// Custom-painted attachment-chip row above the composer, bound to
// ComposerSessionController's ComposerAttachmentModel. Mirrors the GUI's
// AttachmentChip repeater: one chip per pending attachment (kind glyph + name).
// Left/Right select a chip; Delete / Backspace / 'x' removes it via
// attachments()->removeAt(). Auto-sizes to 1 line when there are attachments and
// 0 (dropping out of the focus cycle) when there are none.
class AttachmentBarView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit AttachmentBarView(Tui::ZWidget* parent = nullptr);

    void setController(ComposerSessionController* controller);

    [[nodiscard]] QSize sizeHint() const override;

    // Mouse: select the chip under the clicked x (chips are laid out left-to-right
    // on one row). Removal stays on the keyboard (Del/Backspace/x).
    void clickAt(QPoint local);

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;
    void keyEvent(Tui::ZKeyEvent* event) override;

private:
    void rebuild();
    [[nodiscard]] int count() const;

    ComposerSessionController* m_controller = nullptr;
    RenderLine m_line;
    int m_sel = 0;
    int m_height = 0;
};
