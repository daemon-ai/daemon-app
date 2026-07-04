// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/wasm_online.h"

#include <utility>

// Compiled on every platform so shared call sites link against a definition; no-op off wasm.
#ifdef __EMSCRIPTEN__

#include <emscripten/emscripten.h>

namespace {

// The connection-service callback to fire when the browser reports connectivity returned. Held in
// a file-static because the JS `online` listener reaches C++ through the C-linkage trampoline
// below (emscripten's html5.h has no online/offline callback, so this is the documented
// JS -> C++ bridge). Single-threaded wasm: no synchronization needed.
std::function<void()> g_onOnline; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

} // namespace

extern "C" {
// Invoked from the JS `window.addEventListener('online', ...)` handler installed below.
// EMSCRIPTEN_KEEPALIVE exports it so `Module['_daemon_app_notify_online']` resolves without any
// extra CMake export list.
EMSCRIPTEN_KEEPALIVE void daemon_app_notify_online() {
    if (g_onOnline) {
        g_onOnline();
    }
}
}

namespace platform {

void installOnlineCallback(std::function<void()> onOnline) {
    g_onOnline = std::move(onOnline);
    // Register the browser `online` listener exactly once; it calls back into the trampoline
    // above. Guarded by a Module flag so repeated installs don't stack listeners. If the export
    // is unexpectedly absent the handler is a silent no-op (the reprobe backoff still recovers,
    // just not instantly), which is an acceptable degradation.
    // clang-format off
    EM_ASM({
        if (!Module.__daemonAppOnlineHooked) {
            Module.__daemonAppOnlineHooked = true;
            window.addEventListener('online', function() {
                var fire = Module['_daemon_app_notify_online'];
                if (fire) { fire(); }
            });
        }
    });
    // clang-format on
}

} // namespace platform

#else // desktop / mobile: OS/Qt handle link state; no browser online event.

namespace platform {

void installOnlineCallback(std::function<void()> onOnline) {
    // No browser online event off wasm; drop the callback. Move-from keeps the by-value sink
    // signature (the wasm impl stores it) clean of performance-unnecessary-value-param.
    [[maybe_unused]] const auto dropped = std::move(onOnline);
}

} // namespace platform

#endif // __EMSCRIPTEN__
