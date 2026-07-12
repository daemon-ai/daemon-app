# ADR-008 addendum — M1 observe-seam FINALIZATION (package A3)

> Placement note: ADR-008 lives in the superproject (`docs/architecture/adr/`). A3 does not touch
> the superproject; the parent appends this text to ADR-008 under a new "M1 finalization (A3)"
> section and flips the status line to **Accepted — coarse signals; lager deleted**.

Status: **Gate 1 re-run on the representative VM TU set. Decision CONFIRMED — coarse signals.
`LagerObserve` and the lager/zug vendoring DELETED.**

## Mandate

ADR-008 required A3 to re-run gate 1 (compile cost) on a REPRESENTATIVE VM TU set (≥5 TUs incl.
one QML-heavy and one TUI consumer, clean + incremental) before deleting the loser. A1's M0 spike
defaulted to coarse but measured only isolated minimal TUs (+74% / ~+3 s per TU) and flagged the
percentage as exaggerated by a tiny baseline; this re-run tests real projection TUs.

## Gate 1 — compile cost (RED for lager, on real VM TUs)

Harness: `tests/mirror/observe_gate1/` — one `vm_tu.cpp` compiled in 5 representative VM shapes ×
{coarse, lager}, driven by `run_gate1.sh`. Each TU includes the mirror lens layer
(`TableModel`/`WindowModel`/`ConversationListModel`) + Qt Core/Gui + (for the QML-heavy TU)
`<QQmlEngine>` + (for the TUI TU) `<QIdentityProxyModel>` + `<Tui/ZWidget.h>`, and constructs a
`Store` through the observe seam. The immer cost is common to both variants; the measured delta is
purely the lager reactive-core header surface (zug + boost::intrusive + lager `state`/`cursor`/
`reader`/`watch`/`commit`). Best-of-3, g++ 15.2, `-std=c++20 -O1`, desktop devShell:

| TU | coarse clean / best | lager clean / best | Δ clean | Δ best |
|---|---|---|---|---|
| conversation list | 8.66 s / 8.52 s | 13.39 s / 13.24 s | +55% | +55% |
| chat window (W) | 9.25 s / 8.63 s | 13.85 s / 13.62 s | +50% | +58% |
| session list | 8.95 s / 8.62 s | 13.12 s / 13.12 s | +47% | +52% |
| QML-heavy | 9.43 s / 9.43 s | 13.66 s / 13.66 s | +45% | +45% |
| TUI consumer | 8.61 s / 8.61 s | 13.40 s / 13.40 s | +56% | +56% |
| **TOTAL (5 TU)** | **44.90 s / 43.81 s** | **67.42 s / 67.04 s** | **+50%** | **+53%** |

ADR-008 thresholds: **≤ 15% clean, ≤ 10% incremental.** Lager is **decisively past both** on the
representative set — and unlike A1's minimal TU, the QML/Tui-laden VM TUs have a substantial
baseline, yet the ratio is still ~+50% and the ABSOLUTE tax is ~+4.5 s/TU. The daily-loop cost the
gate exists to bound is real and multiplies across the VM set. Gate 1 is RED for lager.

## Decision

**Coarse signals — confirmed; lager deleted.** The measurement AGREES with A1 (it does not
contradict it), so the loser is removed at M1 per ADR-008's "deferred-decision cost" consequence:

- **Deleted** `src/core/mirror/observe_lager.h`.
- **Retargeted** the lager-only test ports (`tst_mirror_observe`) at the coarse seam: the
  transactional-visibility / glitch-freedom invariants become "one atomic snapshot + exactly one
  `mirrorCommitted` per non-empty batch, published as the fully-applied truth" and "per-reader
  value-compare gating (the coarse equivalent of lager's `has_changed`) over the immer snapshot".
- **Flake-input disposition:** after the deletion NOTHING in the tree used lager or zug (lager was
  used only by `observe_lager.h`, the lager test ports, and the WASM spike probe — all spike
  residue; zug was only lager's transitive dep). The WASM probe (`tests/wasm/mirror_wasm_probe.cpp`)
  was reduced to **immer-only** (it now validates the substrate that actually ships). The `lager`
  and `zug` flake inputs, their `Dependencies.cmake` INTERFACE targets, the `BOOST_INCLUDE_DIR`
  wiring (needed only for lager's `boost::intrusive`), and the `*_SOURCE_DIR` devShell/wasm
  exports were all **removed**. immer is the only remaining mirror dependency. The substrate gate
  `DAEMON_APP_HAVE_MIRROR_SUBSTRATE` now keys on `IMMER_SOURCE_DIR` alone.

## Consequences (per ADR-008)

- The seam confines the choice to scalar/derived plumbing; collection projections consume journal
  deltas regardless (`mirror::TableModel`, §8.1), so deleting lager costs the mirror nothing
  structural. The `mirror::Observe` interface is retained (one conforming impl, `CoarseObserve`) so
  a future re-decision remains a seam swap, not a rewrite.
- **Simplicity fence (enforced):** the moment a derived-node graph is wanted on top of coarse
  signals, this ADR is REOPENED rather than the graph hand-built (01§6's bug class). Re-adopting
  lager then means re-vendoring the two flake inputs — a deliberate, reviewable act.
