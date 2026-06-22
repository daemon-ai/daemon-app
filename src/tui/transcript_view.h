#pragma once

#include "transcript_render.h"

#include <Tui/ZWidget.h>

#include <QString>
#include <QVariantMap>
#include <QVector>

namespace be {
class DocumentStore;
}

// The TUI transcript pane: a scrollable, custom-painted widget that renders a
// be::DocumentStore via TranscriptLayout (the GUI's parse/view-model engine). It
// replaces the read-only ZTextEdit dump, so the store's agent fences become
// colored tool/reasoning cards, ANSI output, and diffs instead of raw JSON.
//
// It owns no document state: the RootWidget feeds it a DocumentStore* (loaded
// from persisted markdown and/or streamed live via be::TranscriptIngest) and
// calls reload() whenever the document changes.
class TranscriptView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit TranscriptView(Tui::ZWidget *parent = nullptr);

    void setDocument(const be::DocumentStore *doc);

    // Rebuild the cached render lines from the document at the current width and
    // repaint. Keeps the view pinned to the bottom while streaming (re-pins on
    // every reload unless the user has scrolled up).
    void reload();

    // Mouse wheel: scroll by `delta` lines (negative = up), re-pinning to the
    // bottom only when the scroll lands there (mirrors the keyboard scroll).
    void scrollByLines(int delta);

    // Mouse click at widget-local point `local`. While an interactive block is
    // present (approval bar / clarify form), a click on a control row focuses it
    // and activates it (buttons decide, choices toggle); a freeform field is just
    // focused for typing. A click elsewhere only focuses the pane (handled by the
    // shell). No-op when there are no controls.
    void clickAt(QPoint local);

signals:
    // The focused approval bar was decided (Enter on [Approve]/[Deny]/[Allow
    // permanently]). `decision` is "approved" / "denied"; `permanent` is the
    // "allow permanently" variant.
    void approvalDecided(const QString &callId, const QString &decision, bool permanent);
    // The focused clarify form was submitted (Enter on [Submit] / a freeform
    // field). `answers` is the canonical collected map (be::clarifyAnswerPatch
    // input).
    void clarifySubmitted(const QString &callId, const QString &requestId,
                          const QVariantMap &answers);

protected:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void keyEvent(Tui::ZKeyEvent *event) override;
    void resizeEvent(Tui::ZResizeEvent *event) override;

private:
    int visibleRows() const;
    int maxScrollTop() const;
    void clampScrollTop();
    void scrollToBottom();
    bool atBottom() const;
    void rebuild();

    // Interactive helpers (active only when m_controls is non-empty).
    bool interactive() const { return !m_controls.isEmpty(); }
    void moveControl(int delta);
    void ensureControlVisible();
    void activateControl(); // Enter on the focused control
    void toggleChoice();    // Space on a focused choice
    QVariantMap toolMetadataForCallId(const QString &callId) const;

    const be::DocumentStore *m_doc = nullptr;
    QVector<RenderLine> m_lines;
    QVector<Control> m_controls;
    // The in-progress clarify answer (radios/checkboxes/freeform), keyed by
    // question id, and the focused control index (-1 = none / scroll mode).
    AnswerDraft m_draft;
    int m_activeControl = -1;
    int m_scrollTop = 0;
    // Keep the newest content visible during streaming; a manual scroll up
    // unpins, End / new content at the bottom re-pins (mirrors the GUI).
    bool m_stickToBottom = true;
};
