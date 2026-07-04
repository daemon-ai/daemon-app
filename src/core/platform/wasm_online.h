// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <functional>

namespace platform {

// Browser (wasm) network-wakeup seam. Safe to include everywhere; the body is a no-op off wasm
// (see wasm_online.cpp). Real behavior is filled in by Stream B (W5).

// Subscribe to the browser `online` event and invoke `onOnline` when connectivity returns, so the
// connection service can trigger an immediate reconnect that skips the current backoff wait.
// TODO(W5): wire emscripten_set_online_callback (or a JS->C++ bridge).
void installOnlineCallback(std::function<void()> onOnline);

} // namespace platform
