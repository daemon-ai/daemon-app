// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

namespace persistence {
class ISessionStore;
}

// A small, front-end-agnostic helper that serializes a session to a JSON
// document and writes text to a local file. The GUI binds it as the `Exporter`
// context property (the /save + session "Export" action); the TUI calls it
// directly. Reads through the ISessionStore seam so the daemon-backed store
// works unchanged.
class TranscriptExporter : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

    // Build a pretty-printed JSON document {id,title,content,exportedAt} for the
    // session in `store` (a QObject* so QML can pass the context property).
    [[nodiscard]] Q_INVOKABLE QString toJson(QObject* store, const QString& sessionId) const;

    // Write `text` to a local-file URL (or plain path). Returns false on failure.
    Q_INVOKABLE bool writeFile(const QUrl& fileUrl, const QString& text) const;

    // Browser (wasm) export: serialize the session to JSON and hand the bytes to the wasm file
    // bridge, which triggers a real browser download (Blob + <a download>). There is no shared
    // filesystem with the node on wasm, so the QtQuick.Dialogs FileDialog + writeFile() path (which
    // lands in the Emscripten sandbox the user can't see) is skipped in favor of this. No-op off
    // wasm — the desktop/TUI keep using writeFile()/exportToPath(). `suggestedName` is the download
    // filename (e.g. "<title-or-id>.json").
    Q_INVOKABLE void exportToBrowser(QObject* store, const QString& sessionId,
                                     const QString& suggestedName) const;

    // Convenience for non-QML callers (the TUI): build + write in one step.
    bool exportToPath(persistence::ISessionStore* store, const QString& sessionId,
                      const QString& filePath) const;
};
