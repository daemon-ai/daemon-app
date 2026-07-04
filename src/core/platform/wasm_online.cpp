// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/wasm_online.h"

#include <utility>

// Compiled on every platform so shared call sites link against a definition; no-op off wasm.
#ifdef __EMSCRIPTEN__

namespace platform {

void installOnlineCallback(std::function<void()> /*onOnline*/) {
    // TODO(W5): subscribe to the browser online event and invoke the callback to fast-reconnect.
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
