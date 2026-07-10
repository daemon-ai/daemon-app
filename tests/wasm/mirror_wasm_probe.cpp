// A1 WASM smoke gate (a) (spec 09 §11; ADR-002/-008): prove the vendored immer + the severable
// lager reactive-core subset COMPILE and RUN single-threaded under the emscripten/wasm toolchain
// with exceptions-off (IMMER auto-detects -fno-exceptions). This is a MINIMAL probe — NOT the
// full app wasm build — driven by tests/wasm/build-wasm-probe.sh under node.
//
// It exercises the load-bearing pieces the mirror leans on: immer::table upsert + immer::diff,
// and lager::state<transactional_tag> + lager::commit two-phase visibility. A nonzero exit or a
// missing sentinel fails the gate.

#include <cstdio>
#include <immer/algorithm.hpp>
#include <immer/table.hpp>
#include <lager/commit.hpp>
#include <lager/state.hpp>
#include <lager/watch.hpp>
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

    // --- lager: transactional two-phase visibility ---
    auto s = lager::make_state(0, lager::transactional_tag{});
    int notifies = 0;
    lager::watch(s, [&](int) { ++notifies; });
    s.set(1);
    s.set(2);
    if (notifies != 0 || s.get() != 0) { // staged, not yet visible
        std::printf("MIRROR_WASM_PROBE fail: premature notify=%d get=%d\n", notifies, s.get());
        return 1;
    }
    lager::commit(s);
    if (notifies != 1 || s.get() != 2) {
        std::printf("MIRROR_WASM_PROBE fail: commit notify=%d get=%d\n", notifies, s.get());
        return 1;
    }

    std::printf("MIRROR_WASM_PROBE ok immer(table+diff) lager(state+commit)\n");
    return 0;
}
