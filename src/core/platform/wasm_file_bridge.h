// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <functional>
#include <QByteArray>
#include <QString>

namespace platform {

// Browser (wasm) file open/save bridge, the QGuiApplication-friendly equivalent of
// QFileDialog::getOpenFileContent / saveFileContent (without linking QtWidgets). Safe to include
// everywhere; the bodies are no-ops off wasm (see wasm_file_bridge.cpp). Real behavior is filled
// in by Stream C (W7/W8).

// Open a browser file picker (<input type="file" accept=acceptFilter>) from a user-gesture
// handler; deliver the chosen file's name + bytes to `cb` asynchronously (no nested event loop).
// On wasm the FileReader result is marshalled back onto the Qt main loop via a queued invocation,
// so `cb` runs in a fresh event-loop turn (never re-entrant from inside the browser event). A
// cancelled picker delivers nothing (the callback is dropped). No-op off wasm.
void openFileContent(const QString& acceptFilter,
                     std::function<void(const QString& name, const QByteArray& bytes)> cb);

// Trigger a browser download of `bytes` with the given suggested filename (Blob + temporary
// <a download> click, then revoke the object URL). No-op off wasm.
void saveFileContent(const QByteArray& bytes, const QString& suggestedName);

// Best-effort clipboard write for the browser (W8). Prefers the async Clipboard API in a secure
// context (https / localhost); otherwise falls back to document.execCommand('copy') via a hidden
// textarea, which MUST be called synchronously inside a user gesture (QML button/shortcut handlers
// qualify). Returns true if any path was taken, false if none worked (so the caller can surface a
// toast). Always false off wasm — desktop/mobile callers use QClipboard directly.
bool writeClipboardText(const QString& text);

} // namespace platform
