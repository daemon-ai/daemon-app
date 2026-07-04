// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

namespace fs {
class IFsService;
}

// Browser (wasm) attachment upload: bridges the local file the user picks in the browser into the
// node workspace over the existing IFsService write seam, then hands back the workspace-relative
// path so the composer can add an @file:/@image: chip the node resolves exactly as on desktop.
//
// On desktop the composer attaches by reference (the daemon shares the filesystem), so this
// controller is wasm-only in practice: openFileContent() is a no-op off wasm and QML gates its use
// on Qt.platform.os === "wasm". Kept all-platform so the QML type resolves everywhere and the
// pure path helper is unit-testable on the desktop ctest run.
class ComposerUploadController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    // The workspace filesystem seam (bind to the `Fs` context property). Writes land through it.
    Q_PROPERTY(QObject* fsService READ fsService WRITE setFsService NOTIFY fsServiceChanged)
    // True from the moment bytes start uploading until the write result (drives in-composer
    // progress); statusText is a short human line ("Uploading <name>…") for the same indicator.
    Q_PROPERTY(bool uploading READ uploading NOTIFY uploadingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    using QObject::QObject;

    // Reject uploads larger than this (browser bytes are held in memory and pushed over the mux).
    static constexpr qint64 kMaxUploadBytes = 25LL * 1024 * 1024;

    QObject* fsService() const;
    void setFsService(QObject* service);
    bool uploading() const { return m_uploading; }
    QString statusText() const { return m_statusText; }

    // Open the browser file picker (accept filter, e.g. "image/*" or ""), read the chosen file,
    // size-guard it, upload it under uploads/<utc-timestamp>-<sanitized-name>, and — on a
    // successful write — emit uploaded(workspacePath, kind). `kind` is the attachment kind the
    // chip should carry ("image" / "file"). Failures emit failed(message).
    Q_INVOKABLE void pickAndUpload(const QString& acceptFilter, const QString& kind);

    // uploads/<utc-timestamp>-<sanitized-name> for `originalName` at `utcNow`. Names are basenamed
    // and reduced to [A-Za-z0-9._-] (others -> '_'); an empty result becomes "file". Static + pure
    // so it is unit-testable without a browser or an fs service.
    [[nodiscard]] static QString uploadPathFor(const QString& originalName,
                                               const QDateTime& utcNow);

signals:
    void uploaded(const QString& workspacePath, const QString& kind);
    void failed(const QString& message);
    void uploadingChanged();
    void statusTextChanged();
    void fsServiceChanged();

private:
    void setUploading(bool uploading);
    void setStatusText(const QString& text);
    // Once a file's bytes are in hand: guard size, then resolve the writable workspace root and
    // write the bytes there, reporting the outcome through uploaded()/failed().
    void beginUpload(const QString& originalName, const QByteArray& bytes, const QString& kind);

    QPointer<fs::IFsService> m_fs;
    bool m_uploading = false;
    QString m_statusText;
};
