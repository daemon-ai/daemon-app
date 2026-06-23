#pragma once

#include "transcript_render.h"

#include <Tui/ZWidget.h>

#include <QString>
#include <QVariantMap>
#include <QVector>

namespace be {
class DocumentStore;
class TranscriptSearchController;
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

    // Bind the active tab's in-transcript find engine so reload() can highlight the
    // query's occurrences (the active match's block gets a stronger wash). May be
    // null (no highlighting). The view does not own it.
    void setSearch(const be::TranscriptSearchController *search);

    // Scroll so the first rendered row of document block `blockIndex` is visible
    // (anchor-style, mirroring ensureAnchorVisible). Unpins stick-to-bottom so the
    // jump holds. No-op for an out-of-range / non-rendered (tombstoned) block.
    void scrollBlockIntoView(int blockIndex);

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
    // Rewind picker actions on the selected prior user-message anchor. Restore
    // re-runs it with its own text; edit asks the host to seed the composer with
    // `text` (after truncating). The host interrupts a live turn and truncates.
    void rewindRestoreRequested(const QString &messageId);
    void rewindEditRequested(const QString &messageId, const QString &text);

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
    // Recolor query occurrences in the cached render lines (called from rebuild
    // when a search is active). Splits affected spans so the matched substring
    // gets a highlight background; the active match's block is emphasised more.
    void applySearchHighlight();

    // Interactive helpers (active only when m_controls is non-empty).
    bool interactive() const { return !m_controls.isEmpty(); }
    void moveControl(int delta);
    void ensureControlVisible();
    void activateControl(); // Enter on the focused control
    void toggleChoice();    // Space on a focused choice
    QVariantMap toolMetadataForCallId(const QString &callId) const;

    // Rewind picker helpers (a selection mode over the user-message anchors,
    // distinct from the interactive-control focus). enterRewind selects the last
    // anchor; moveRewind walks between anchors; exitRewind leaves the mode. Active
    // only when m_rewindMode is on and the document has at least one anchor.
    bool rewindActive() const { return m_rewindMode && !m_anchors.isEmpty(); }
    void enterRewind();
    void exitRewind();
    void moveRewind(int delta);
    void ensureAnchorVisible();

    const be::DocumentStore *m_doc = nullptr;
    const be::TranscriptSearchController *m_search = nullptr;
    QVector<RenderLine> m_lines;
    QVector<Control> m_controls;
    // Block <-> line maps from the last build (see LayoutResult), used to scroll a
    // find match into view and to emphasise the active match's block.
    QVector<int> m_blockFirstLine;
    QVector<int> m_lineBlock;
    // Rewind anchors (prior user messages) and the picker's current selection.
    QVector<Anchor> m_anchors;
    bool m_rewindMode = false;
    int m_rewindIndex = -1;
    // The in-progress clarify answer (radios/checkboxes/freeform), keyed by
    // question id, and the focused control index (-1 = none / scroll mode).
    AnswerDraft m_draft;
    int m_activeControl = -1;
    int m_scrollTop = 0;
    // Keep the newest content visible during streaming; a manual scroll up
    // unpins, End / new content at the bottom re-pins (mirrors the GUI).
    bool m_stickToBottom = true;
};
