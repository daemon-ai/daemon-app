// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

namespace platform {

// Browser (wasm) persistence seams. The header is safe to include on every platform; the bodies
// are no-ops off wasm (see wasm_persistence.cpp) so shared call sites (e.g. DaemonCacheStore write
// paths) need no #ifdef. Real behavior is filled in by Stream A (W1/W2).

// Kick a debounced FS.syncfs(false, ...) so the IDBFS-mounted AppData dir (daemon_cache*.db) is
// written back to IndexedDB. Called from the cache write/upsert paths; coalesces bursts behind a
// single-shot timer. TODO(W2): implement the debounce + EM_ASM syncfs.
void scheduleIdbfsSync();

// Register a browser beforeunload/pagehide handler that flushes QtSettingsStore + UiSettings and
// kicks a final FS.syncfs before the tab closes. TODO(W1): implement the unload hook.
void installUnloadFlush();

} // namespace platform
