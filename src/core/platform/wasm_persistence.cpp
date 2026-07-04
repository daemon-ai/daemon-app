// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/wasm_persistence.h"

// Unlike wasm_page_origin.cpp (wasm-only sources), this TU is compiled on every platform so the
// all-platform call sites (DaemonCacheStore) link against a definition everywhere; the body is a
// no-op off wasm.
#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <QObject>
#include <QTimer>

namespace platform {

namespace {

// ~3 s single-shot debounce: cache writes arrive in bursts (a roster refresh upserts many rows),
// and FS.syncfs(false) walks the whole IDBFS mount, so we coalesce a burst into one write-back.
constexpr int kIdbfsSyncDebounceMs = 3000;

// Lazily-constructed on first scheduleIdbfsSync() (well after QApplication exists, since it fires
// from cache write paths). Leaked deliberately: it must outlive every caller for the process
// lifetime, and there is no clean owner on the wasm main thread.
QTimer* idbfsSyncTimer() {
    static QTimer* const timer = [] {
        auto* t = new QTimer; // NOLINT(cppcoreguidelines-owning-memory): process-lifetime singleton
        t->setSingleShot(true);
        t->setInterval(kIdbfsSyncDebounceMs);
        QObject::connect(t, &QTimer::timeout, [] {
            // FS is in scope inside EM_ASM (the emscripten runtime's own symbol), so no
            // EXPORTED_RUNTIME_METHODS entry is needed here — only the preRun mount in the shell,
            // which runs outside the module, needs FS/addRunDependency exported.
            // clang-format off
            EM_ASM({
                if (typeof FS !== 'undefined' && FS.syncfs) {
                    FS.syncfs(false, function(err) {
                        if (err) { console.warn('daemon: FS.syncfs(false) failed', err); }
                    });
                }
            });
            // clang-format on
        });
        return t;
    }();
    return timer;
}

} // namespace

void scheduleIdbfsSync() {
    idbfsSyncTimer()->start(); // (re)arm the debounce window
}

void installUnloadFlush() {
    // Idempotent: a single set of listeners for the process lifetime (the cache store constructs
    // once, but guard anyway so a second caller cannot stack duplicate handlers).
    static bool installed = false;
    if (installed) {
        return;
    }
    installed = true;
    // Force a final, non-debounced write-back of the IDBFS-mounted AppData dir when the tab is
    // hidden or torn down, so the last cache mutations reach IndexedDB. Best-effort: FS.syncfs is
    // async and an abrupt close may cut it short, but visibilitychange->hidden (backgrounding, the
    // common case) and pagehide give it a chance. Settings need no flush here: QtSettingsStore and
    // UiSettings both sync() after every write (WebLocalStorageFormat is synchronous), so their
    // state is already durable by the time this fires.
    // clang-format off
    EM_ASM({
        var flush = function() {
            try {
                if (typeof FS !== 'undefined' && FS.syncfs) {
                    FS.syncfs(false, function() {});
                }
            } catch (e) { /* nothing actionable during teardown */ }
        };
        window.addEventListener('pagehide', flush);
        document.addEventListener('visibilitychange', function() {
            if (document.visibilityState === 'hidden') { flush(); }
        });
    });
    // clang-format on
}

} // namespace platform

#else // desktop / mobile: nothing to persist to a browser origin.

namespace platform {

void scheduleIdbfsSync() {}
void installUnloadFlush() {}

} // namespace platform

#endif // __EMSCRIPTEN__
