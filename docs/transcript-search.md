# In-transcript search - architecture & wiring

Status: **engine implemented + unit-tested (feasibility spike); front-end UI wiring designed, not yet built.**

This is the "Search in transcript (and navigate to, anchor-style)" feature. The
spike delivered in this task is the shared match engine plus its unit test
(`tests/unit/tst_transcript_search.cpp`); the in-block highlight UI in each front
end is designed here and deferred (see *Deferred / risks*).

## Shared engine - `be::TranscriptSearchController`

`src/DaemonApp/BlockEditor/core/transcript_search.{h,cpp}` (in `be_core`, so both
front ends link the identical class, exactly like `be::DocumentStore` itself).

It is toolkit-agnostic: a `QObject` with `Q_PROPERTY`/`Q_INVOKABLE` surface so QML
can bind it directly, and a plain C++ API the TUI calls. It owns no rendering.

```
setDocument(be::DocumentStore*)        // what to search (no change signal -> refresh())
query / setQuery(QString)              // re-collects on change
caseSensitive / setCaseSensitive(bool) // re-collects on change
refresh()                              // re-collect for the current query+document
clear()                                // drop query + matches

matchCount : int                       // total matches
currentMatch : int                     // active match index, -1 when none
next() / previous() / setCurrent(i)    // move the cursor (wraps)

currentBlockIndex()/currentCharStart()/currentCharLen()
matchBlockIndex(i)/matchCharStart(i)/matchCharLen(i)
searchableText(blockIndex) -> QString  // the visible text a block contributes

signal navigateTo(int blockIndex, int charOffset)  // anchor: scroll this into view
signals matchesChanged / currentMatchChanged / queryChanged / caseSensitiveChanged
```

### Match model

A match is `{ blockIndex, charStart, charLen }`:

- `blockIndex` is the `DocumentStore` model row (`blockAt(row)`), i.e. the same row
  the GUI `BlockModel` and the TUI `TranscriptView` already address. This is the
  key property the spike de-risks: match -> block row is a direct identity, so
  "scroll to the match" is just "scroll to that row".
- `charStart`/`charLen` are **UTF-16 (`QString`) offsets within that block's
  searchable text** - the units a front end needs to highlight a sub-range.

Matches are collected in document order (row ascending, then left-to-right within
a block) and are non-overlapping. `refresh()` re-anchors the cursor to the first
match at/after the previously active block, so a streaming re-collect doesn't snap
back to match 0.

### What "searchable text" means

`searchableTextFor(block)` searches what the **user reads**, not the serialized
form:

- Reasoning / Content blocks -> `metadata["body"]` (their visible body, not the
  fenced `\`\`\`reasoning` JSON envelope).
- ToolCall blocks -> `metadata["argsSummary"]` (the one-line summary shown on the
  chip).
- Everything else (paragraph, heading, list, quote, code) -> `block.markdown()`.

This is the one place to extend if a block type should expose more text to search
(e.g. tool stdout); the offsets a front end highlights are always relative to the
string this returns, which `searchableText(row)` re-exposes for that purpose.

## GUI wiring (QML)

1. **Expose the controller.** Register `be::TranscriptSearchController` as a
   `QML_ELEMENT` (thin `DaemonApp.*` wrapper, or add it to the BlockEditor module
   alongside `EditorController`). `TranscriptPage.qml` owns one instance and calls
   `setDocument(editorController.documentStore)` (C++-side wiring, since the
   document pointer isn't a QML value). Search is **per tab**, living on the page
   that owns the transcript's controller/store.
2. **Search bar.** `Ctrl+F` (and an entry behind the Settings `...` menu) toggles a
   one-line search field overlaid on the transcript header area: text field bound
   to `query`, a `"3/12"` match counter (`currentMatch+1`/`matchCount`), up/down
   buttons -> `previous()`/`next()`, Esc -> `clear()` + hide. Enter / Shift+Enter
   also map to `next()`/`previous()`.
3. **Anchor scroll.** Connect `navigateTo(blockIndex, _)` ->
   `transcriptList.positionViewAtIndex(blockIndex, ListView.Contain)` (the same
   anchor-scroll the heading-link navigation already uses via
   `rowForHeadingAnchor`). This part works today with no delegate changes.
4. **In-block highlight (deferred).** `BlockDelegate` rich text has no
   arbitrary-range highlight today. Plan: pass the active match's
   `(charStart, charLen)` for `blockIndex == currentBlockIndex()` into the delegate
   and have the inline projector emit a highlight span (reuse the selection/render
   span machinery). This is the hard part and is out of the spike.

## TUI wiring (`RootWidget` / `TranscriptView`)

1. **Per-tab controller.** Add a `TranscriptSearchController` to each `TabSession`
   (it already owns that tab's `DocumentStore`); `setDocument(&session.doc)` when
   the session is created, `refresh()` after the transcript changes.
2. **Search field.** `Ctrl+F` / `Ctrl+S` opens a one-line search field (reuse
   `SearchInputBox` styling, same pattern as the composer's reverse-history
   search). Live-bind typing to `setQuery`; `n` / `N` / Enter / Shift+Enter ->
   `next()`/`previous()`; Esc -> `clear()` + close. Route the keys in
   `RootWidget::keyEvent` next to the existing `Ctrl+T`/`Ctrl+W`/`Ctrl+Tab`
   handling.
3. **Anchor scroll + highlight.** On `navigateTo(blockIndex, charOffset)`,
   `TranscriptView` scrolls so `blockIndex` is visible (it already maps block rows
   to `RenderLine`s). Because the TUI paints its own `RenderLine` spans, the
   matched range can be highlighted directly (map `charStart/charLen` onto the
   block's render spans and invert/emphasise) - more tractable than the GUI rich
   text, so the TUI can ship highlight earlier.

## Proving it works

`tests/unit/tst_transcript_search.cpp` (links `be_core`, runs in the fast unit
suite) covers: empty query, document-order collection with exact block/offset
geometry, multiple matches in one block, case sensitivity, `next`/`previous`
wrap + `navigateTo` payloads, `setQuery` auto-navigating to the first match,
searching a typed reasoning block's visible body, and `clear()`.

## Deferred / risks

- **In-block highlight rendering** is the only non-trivial remaining piece and the
  reason search shipped as design + spike. GUI needs a highlight-range span in
  `BlockDelegate`/the inline projector; TUI needs span emphasis in `TranscriptView`.
- **Incremental refresh during streaming:** today `refresh()` re-collects the whole
  document. Fine at transcript sizes; if it ever matters, collection can be scoped
  to changed rows using the `BlockChangeSet` the store already emits.
- **Match unit:** offsets are UTF-16; a front end mapping to grapheme/byte columns
  (e.g. for cell-accurate TUI highlight of wide glyphs) must convert.
