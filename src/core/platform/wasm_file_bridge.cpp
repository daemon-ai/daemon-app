// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/wasm_file_bridge.h"

#include <utility>

// Compiled on every platform so shared call sites link against a definition; no-op off wasm (the
// desktop file flows keep using QtQuick.Dialogs FileDialog + QFile).
#ifdef __EMSCRIPTEN__

namespace platform {

void openFileContent(const QString& /*acceptFilter*/,
                     std::function<void(const QString& name, const QByteArray& bytes)> /*cb*/) {
    // TODO(W7): <input type="file"> picker + FileReader; deliver {name, bytes} via a queued signal.
}

void saveFileContent(const QByteArray& /*bytes*/, const QString& /*suggestedName*/) {
    // TODO(W7): Blob + temporary <a download> click, then URL.revokeObjectURL.
}

} // namespace platform

#else // desktop / mobile: real filesystem via FileDialog + QFile; this bridge is unused.

namespace platform {

void openFileContent(const QString& /*acceptFilter*/,
                     std::function<void(const QString& name, const QByteArray& bytes)> cb) {
    // No browser picker off wasm; drop the callback. Move-from keeps the by-value sink signature
    // (the wasm impl stores it) clean of performance-unnecessary-value-param.
    [[maybe_unused]] const auto dropped = std::move(cb);
}

void saveFileContent(const QByteArray& /*bytes*/, const QString& /*suggestedName*/) {}

} // namespace platform

#endif // __EMSCRIPTEN__
