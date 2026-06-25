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

    // Convenience for non-QML callers (the TUI): build + write in one step.
    bool exportToPath(persistence::ISessionStore* store, const QString& sessionId,
                      const QString& filePath) const;
};
