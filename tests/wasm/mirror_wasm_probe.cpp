// WASM smoke gate (spec 09 §11; ADR-002/-008): prove the vendored immer value substrate COMPILES
// and RUNS single-threaded under the emscripten/wasm toolchain with exceptions-off (IMMER
// auto-detects -fno-exceptions). This is a MINIMAL probe — NOT the full app wasm build — driven
// by tests/wasm/build-wasm-probe.sh under node.
//
// It exercises the load-bearing pieces the mirror leans on: immer::table upsert + immer::diff.
// (The lager reactive-core probe was removed when ADR-008 was FINALIZED to the coarse-signals
// seam — A3's gate-1 re-run on the representative VM TU set stayed decisively RED for lager; see
// src/core/mirror/ADR-008-addendum-A3.md. The mirror no longer vendors lager, so the wasm gate
// tracks only the substrate that ships.) A nonzero exit or a missing sentinel fails the gate.

#include <cstdio>
#include <immer/algorithm.hpp>
#include <immer/table.hpp>
#include <string>

namespace {

struct Row {
    std::string id;
    std::string value;
};
struct RowKeyFn {
    std::string operator()(const Row& r) const { return r.id; }
};
bool operator==(const Row& a, const Row& b) {
    return a.id == b.id && a.value == b.value;
}

using Table = immer::table<Row, RowKeyFn>;

} // namespace

int main() {
    // --- immer: upsert + structural diff ---
    Table t;
    t = t.insert(Row{"a", "1"});
    t = t.insert(Row{"b", "1"});
    const Table before = t;
    t = t.insert(Row{"b", "2"});   // change
    t = t.insert(Row{"c", "1"});   // add
    t = t.erase(std::string{"a"}); // remove

    int added = 0, removed = 0, changed = 0;
    immer::diff(before, t,
                immer::make_differ([&](const Row&) { ++added; }, [&](const Row&) { ++removed; },
                                   [&](const Row&, const Row&) { ++changed; }));
    if (added != 1 || removed != 1 || changed != 1) {
        std::printf("MIRROR_WASM_PROBE fail: diff a=%d r=%d c=%d\n", added, removed, changed);
        return 1;
    }

    std::printf("MIRROR_WASM_PROBE ok immer(table+diff)\n");
    return 0;
}
