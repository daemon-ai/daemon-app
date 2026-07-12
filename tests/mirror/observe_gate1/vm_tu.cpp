// Representative VM translation unit for the ADR-008 gate-1 RE-MEASUREMENT (package A3, spec
// 09 §4.2). A1 ran gate 1 on isolated minimal TUs and defaulted to COARSE; A3 re-confirmed on
// this REPRESENTATIVE VM TU set (5 shapes incl. a QML-heavy and a TUI consumer) before deleting
// the loser. Recorded result (best-of-3, g++ 15.2, -std=c++20 -O1, desktop devShell):
//
//   TU             coarse (clean/best)   lager (clean/best)   d clean   d best
//   conv-list           8.66s / 8.52s        13.39s / 13.24s    +55%      +55%
//   chat-window         9.25s / 8.63s        13.85s / 13.62s    +50%      +58%
//   session-list        8.95s / 8.62s        13.12s / 13.12s    +47%      +52%
//   QML-heavy           9.43s / 9.43s        13.66s / 13.66s    +45%      +45%
//   TUI-consumer        8.61s / 8.61s        13.40s / 13.40s    +56%      +56%
//   TOTAL (5 TU)       44.90s /43.81s        67.42s / 67.04s    +50%      +53%
//
// Gate-1 thresholds: <=15% clean, <=10% incremental (ADR-008). Lager stayed DECISIVELY RED, so
// LagerObserve + the lager/zug flake inputs were DELETED and the seam finalized to coarse. This
// TU now compiles COARSE-ONLY (the lager candidate no longer exists) and stands as a
// representative "VM projection TU builds" smoke; the historical lager numbers above are the
// preserved evidence (also in src/core/mirror/ADR-008-addendum-A3.md).

#include "mirror/conversation_list_model.h"
#include "mirror/observe_coarse.h"
#include "mirror/store.h"
#include "mirror/table_model.h"
#include "mirror/window_model.h"

using ObserveImpl = mirror::CoarseObserve;

// TU_KIND: 1 conversation list, 2 chat window, 3 session list, 4 QML-heavy, 5 TUI consumer.
#ifndef TU_KIND
#define TU_KIND 1
#endif

#if TU_KIND == 4
#include <QQmlEngine> // QML-heavy VM TU (registers/exposes a model to QML)
#endif
#if TU_KIND == 5
#include <QIdentityProxyModel> // the DisplayRoleAdapter base (TUI consumer)
#include <Tui/ZWidget.h>       // representative Tui Widgets surface
#endif

#include <QString>

// A representative VM assembly for this TU kind. Both seams construct + drive the Store through
// the observe seam so the seam header is fully compiled (the coarse/lager delta is the tax the
// gate bounds).
int gate1_probe_entry() {
    ObserveImpl obs;
    mirror::Store store(obs);

#if TU_KIND == 1
    mirror::ConversationListModel m(store);
    return m.rowCount();
#elif TU_KIND == 2
    mirror::WindowModel<mirror::ChatMessage> m(
        store, mirror::ChatMessageScope{QStringLiteral("t"), QStringLiteral("c")});
    return m.rowCount();
#elif TU_KIND == 3
    mirror::TableModel<mirror::Session> m(store);
    return m.rowCount();
#elif TU_KIND == 4
    mirror::TableModel<mirror::Conversation> m(store);
    QQmlEngine engine;
    (void)engine;
    return m.rowCount();
#elif TU_KIND == 5
    mirror::ConversationListModel base(store);
    QIdentityProxyModel proxy;
    proxy.setSourceModel(&base);
    return proxy.rowCount();
#else
#error "unknown TU_KIND"
#endif
}
