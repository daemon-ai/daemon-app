// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QList>
#include <QString>
#include <QtLogging>

namespace daemonapp::daemon {

// The shared client half of the uniform wire pagination (wire v25 `WirePage` / `after` cursors):
// a minimal accumulate + runaway-guard state for the "keep re-issuing with the previous page's
// `next` until it clears" loop every paged repository runs. Deliberately tiny — no QObject, no
// signals, no request plumbing: the owning repository keeps its encode/decode/merge glue and this
// struct keeps the loop's state so the pattern is written once.
//
// Usage per received page:
//   loop.items.append(pageItems);
//   if (!next.isEmpty() && loop.guard(next)) { re-issue with `after = next`; return; }
//   consume loop.items; loop.reset();
template <typename Row>
struct PageLoop {
    // Everything accumulated so far (across the pages received to date).
    QList<Row> items;
    // Pages already received (the guard's counter).
    int pages = 0;
    // The cursor the loop last re-issued as `after` (empty until the first continuation), so a
    // non-advancing server cursor is detected instead of looped on.
    QString lastCursor;
    // Runaway guard: stop after this many pages (32k rows at the 64/page wire bound) and serve
    // what accumulated rather than looping forever on a pathological/hostile cursor chain.
    static constexpr int kMaxPages = 512;

    // Account one received page whose resume cursor is `next`; true while the loop may fetch
    // another page with `after = next`. False (stop and serve what accumulated) when the page
    // budget is exhausted, or when `next` equals the cursor this loop just sent — a server that
    // echoes a non-advancing cursor would otherwise spin the loop forever, so a cursor bug
    // degrades into one wasted page instead of a request storm.
    [[nodiscard]] bool guard(const QString& next) {
        if (++pages > kMaxPages) {
            qWarning("PageLoop: page budget exhausted (%d pages); serving what accumulated",
                     kMaxPages);
            return false;
        }
        if (!next.isEmpty() && next == lastCursor) {
            qWarning("PageLoop: pagination cursor did not advance; stopping the page loop");
            return false;
        }
        lastCursor = next;
        return true;
    }

    // Drop the loop state (call when the loop completes, dies, or is superseded).
    void reset() {
        items.clear();
        pages = 0;
        lastCursor.clear();
    }
};

} // namespace daemonapp::daemon
