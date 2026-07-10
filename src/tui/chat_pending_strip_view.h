// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transcript_render.h" // Span / RenderLine

#include <QVector>
#include <Tui/ZWidget.h>

class ConvSendController;

// mirror A5 (spec 09 §8.4/§6.5) — the TUI chat pending strip: the per-conversation outbox lens
// rendered BESIDE the timeline (docked above the composer, the QueueStripView idiom). Bound to
// the ACTIVE chat tab's ConvSendController; 0-height whenever the bound lane has no pending ops
// (or no chat tab is active), dropping out of the focus cycle. Pending ops are visibly distinct
// from node-confirmed transcript rows by construction (they render HERE, never in the
// transcript, R2), with per-state markers: · queued, ↑ sending, ✓ sent-awaiting, ✗ rejected
// (+ the node's error text). Keyboard mirrors QueueStripView and carries the §6.5 affordances:
// Up/Down select; Enter / 's' drain ("send now", §6.8 manual drain); 'r' retry; 'e' edit (loads
// the text into the composer via editRequested); 'd' / Delete discard; 'a' send-remaining-anyway
// on a paused lane.
class ChatPendingStripView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit ChatPendingStripView(Tui::ZWidget* parent = nullptr);

    // Rebind to the active chat tab's controller (null = no chat tab active → collapse).
    void setController(ConvSendController* controller);

    [[nodiscard]] QSize sizeHint() const override;

signals:
    // 'e' took a rejected entry for edit: the shell loads `text` into the composer + focuses it
    // (the edited send mints a NEW op-id, §6.5).
    void editRequested(const QString& text);

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;
    void keyEvent(Tui::ZKeyEvent* event) override;
    void resizeEvent(Tui::ZResizeEvent* event) override;

private:
    void rebuild();     // height + parent relayout on data change
    void layoutLines(); // rebuild span lines for the current width
    [[nodiscard]] int count() const;
    [[nodiscard]] QString opIdAt(int row) const;
    [[nodiscard]] int stateAt(int row) const;

    ConvSendController* m_controller = nullptr;
    QVector<RenderLine> m_lines;
    int m_sel = 0;
    int m_height = 0;

    static constexpr int kMaxRows = 4;
};
