// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/wasm_persistence.h"

// Unlike wasm_page_origin.cpp (wasm-only sources), this TU is compiled on every platform so the
// all-platform call sites (DaemonCacheStore) link against a definition everywhere; the body is a
// no-op off wasm.
#ifdef __EMSCRIPTEN__

namespace platform {

void scheduleIdbfsSync() {
    // TODO(W2): debounced FS.syncfs(false, ...) via a single-shot QTimer + EM_ASM.
}

void installUnloadFlush() {
    // TODO(W1): register beforeunload/pagehide -> QtSettingsStore::sync() + UiSettings sync +
    // final FS.syncfs.
}

} // namespace platform

#else // desktop / mobile: nothing to persist to a browser origin.

namespace platform {

void scheduleIdbfsSync() {}
void installUnloadFlush() {}

} // namespace platform

#endif // __EMSCRIPTEN__
