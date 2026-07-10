# ADR-008 addendum — M0 observe-seam spike results (package A1)

> Placement note: ADR-008 lives in the superproject (`docs/architecture/adr/`). A1 does not
> touch the superproject; the parent appends this text to ADR-008 under a new
> "M0 spike results (A1)" section and flips the status line.

Status: **Spike run. Decision — default to coarse signals (candidate b).** Both
implementations remain behind `mirror::Observe` through M0; the unused one is deleted at M1
(per this ADR's "deferred-decision cost" consequence).

## What was built

Both conforming implementations of the `mirror::Observe` seam (spec §4.2) exist and are
exercised by `tst_mirror_observe`:

- **(a) `LagerObserve`** — `lager::state<MirrorModel, transactional_tag>` + `lager::commit`;
  the severable reactive-core header subset only (`state`/`commit`/`cursor`/`reader`/`watch`),
  never `lager::store`/effects/debugger. Pulls `zug` + header-only `boost::intrusive`
  (verified: NOT `boost::hana` — the compiled subset links neither).
- **(b) `CoarseObserve`** — a single `QObject` with one `mirrorCommitted(revFrom, revTo)` signal;
  per-VM value reads + value-compare gating.

## Gate 1 — compile cost (RED for lager)

Measured on the desktop preset (gcc 15.2, `-std=c++20`, ccache cold), two otherwise-identical
minimal TUs that include `mirror/store.h` + `mirror/model.h` (immer) and differ only in the
observe header. Best-of-3 wall clock:

| TU | best compile |
|---|---|
| coarse (`observe_coarse.h`) | **4.40 s** |
| lager (`observe_lager.h`)   | **7.67 s** |

Delta: **+3.27 s (+74%) per TU** attributable to the lager reactive-core + zug + boost::intrusive
header surface. ADR-008's gate-1 threshold is **≤ 15% clean-build wall-clock increase**. The
isolated observe TU is **far** past that.

Honesty caveat: an isolated minimal TU exaggerates the *percentage* (the non-lager baseline
work is small). A heavier real VM TU (Qt/QML-laden) would show a smaller ratio, but the
**absolute** ~3 s/TU tax is real and multiplies across the ≥5-TU VM set the gate targets, and it
is the daily-loop cost the gate exists to bound. A3 should re-run gate 1 on the representative VM
TU set at M1 before deleting the loser; the substrate default below stands regardless.

## Gate 2 — WASM smoke (immer + lager under emscripten)

Probe: `tests/wasm/mirror_wasm_probe.cpp` (immer `table` upsert + `diff`; lager `state` +
`commit` two-phase visibility), built with `em++ -std=c++20 -Oz -fno-exceptions -fno-rtti
-DIMMER_NO_THREAD_SAFETY=1` and run under node — see `tests/wasm/build-wasm-probe.sh`.

Result: **<FILL FROM RUN>** — the `.#wasm` devShell first builds the Qt-for-WebAssembly
toolchain from source (large; see the honest-cost note in the A1 report). The pure immer+lager
probe needs only `em++` + the header include paths (no Qt), so it runs as soon as the emscripten
toolchain is available. immer ships upstream emscripten bindings and lager's core is
platform-free (01§3.6), so no obstacle is expected; the sentinel `MIRROR_WASM_PROBE ok` is the
pass condition.

## Gate — code weight

- coarse: ~40 lines, one signal, zero transitive template surface.
- lager: ~60 lines wrapper **plus** zug + boost::intrusive as transitive includes of every VM TU
  that touches a scalar reader; plus the pre-1.0 vendor-and-own maintenance posture (01§5) and
  the no-cursors-in-callbacks reentrancy rule (§14.9).

## Decision & rationale

**Default: coarse signals.** ADR-008 says "default to lager headers **iff both gates are
green**." Gate 1 is decisively not green for lager. The seam confines the choice to scalar/
derived plumbing — collection row deltas consume the journal either way (§8.1) — so choosing
coarse costs the mirror nothing structural. Equality gating still happens per-reader by immer
value compare (cheap). The ADR's simplicity fence applies: the moment a derived-node graph is
wanted on top of coarse signals, reopen this ADR rather than hand-building the graph.

`LagerObserve` is retained through M0 (behind the seam, compiling, tested) so A3/M1 can confirm
gate 1 on the full VM TU set and re-decide if the representative measurement contradicts this;
otherwise it is deleted at M1.

Auxiliary observations (non-gating): `extra/qt.hpp` guards moc with `#ifndef Q_MOC_RUN` (fine
under AUTOMOC); the lager transactional-set / batched-commit / derived-reader ports pass on
desktop, confirming glitch-free two-phase behavior is available if a future re-decision wants it.
