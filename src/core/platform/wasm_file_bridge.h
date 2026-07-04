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
// TODO(W7): implement via EM_ASM/emscripten::val + FileReader, marshalling back through a queued
// signal.
void openFileContent(const QString& acceptFilter,
                     std::function<void(const QString& name, const QByteArray& bytes)> cb);

// Trigger a browser download of `bytes` with the given suggested filename (Blob + temporary
// <a download> click, then revoke the object URL). TODO(W8): implement via EM_ASM.
void saveFileContent(const QByteArray& bytes, const QString& suggestedName);

} // namespace platform
