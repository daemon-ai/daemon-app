// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "composer_upload_controller.h"

#include "fs/ifs_service.h"
#include "platform/wasm_contracts.h"
#include "platform/wasm_file_bridge.h"

#include <memory>
#include <QList>

namespace {

// Reduce a picked filename to a safe, workspace-relative leaf: basename only, characters outside
// [A-Za-z0-9._-] collapsed to '_'. Mirrors the workspace-key sanitization used elsewhere.
QString sanitizeLeaf(const QString& name) {
    // Basename: drop anything up to the last POSIX or Windows separator the browser might report.
    qsizetype cut = -1;
    for (qsizetype i = 0; i < name.size(); ++i) {
        if (name.at(i) == QLatin1Char('/') || name.at(i) == QLatin1Char('\\')) {
            cut = i;
        }
    }
    const QString base = (cut >= 0) ? name.mid(cut + 1) : name;

    QString out;
    out.reserve(base.size());
    for (const QChar ch : base) {
        const bool safe = (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z')) ||
                          (ch >= QLatin1Char('a') && ch <= QLatin1Char('z')) ||
                          (ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) ||
                          ch == QLatin1Char('.') || ch == QLatin1Char('_') ||
                          ch == QLatin1Char('-');
        out.append(safe ? ch : QLatin1Char('_'));
    }
    return out.isEmpty() ? QStringLiteral("file") : out;
}

} // namespace

QObject* ComposerUploadController::fsService() const {
    return m_fs;
}

void ComposerUploadController::setFsService(QObject* service) {
    auto* fsService = qobject_cast<fs::IFsService*>(service);
    if (m_fs == fsService) {
        return;
    }
    m_fs = fsService;
    emit fsServiceChanged();
}

void ComposerUploadController::setUploading(bool uploading) {
    if (m_uploading == uploading) {
        return;
    }
    m_uploading = uploading;
    emit uploadingChanged();
}

void ComposerUploadController::setStatusText(const QString& text) {
    if (m_statusText == text) {
        return;
    }
    m_statusText = text;
    emit statusTextChanged();
}

QString ComposerUploadController::uploadPathFor(const QString& originalName,
                                                const QDateTime& utcNow) {
    const QString stamp = utcNow.toUTC().toString(QStringLiteral("yyyyMMdd'T'HHmmsszzz"));
    return QString::fromLatin1(platform::kUploadsDirPrefix) + stamp + QLatin1Char('-') +
           sanitizeLeaf(originalName);
}

void ComposerUploadController::pickAndUpload(const QString& acceptFilter, const QString& kind) {
    if (m_fs.isNull()) {
        emit failed(tr("Not connected to a workspace."));
        return;
    }
    // openFileContent delivers {name, bytes} on a fresh event-loop turn (no nested loop); a
    // cancelled picker delivers nothing.
    platform::openFileContent(acceptFilter,
                              [this, kind](const QString& name, const QByteArray& bytes) {
                                  beginUpload(name, bytes, kind);
                              });
}

void ComposerUploadController::beginUpload(const QString& originalName, const QByteArray& bytes,
                                           const QString& kind) {
    if (m_fs.isNull()) {
        emit failed(tr("Not connected to a workspace."));
        return;
    }
    if (bytes.size() > kMaxUploadBytes) {
        emit failed(tr("\"%1\" is too large to upload (max 25 MB).").arg(originalName));
        return;
    }

    const QString path = uploadPathFor(originalName, QDateTime::currentDateTimeUtc());
    setStatusText(tr("Uploading %1…").arg(originalName));
    setUploading(true);

    fs::IFsService* fsService = m_fs;
    // Resolve the writable workspace root (Host browse roots are read-only), then write the bytes
    // there. Each step is a single-shot connection scoped to this upload so concurrent uploads and
    // the (re-emitting) rootsChanged/writeResult signals don't clobber one another.
    auto rootsConn = std::make_shared<QMetaObject::Connection>();
    *rootsConn =
        connect(fsService, &fs::IFsService::rootsChanged, this,
                [this, fsService, path, kind, bytes, rootsConn](const QList<fs::FsRoot>& roots) {
                    disconnect(*rootsConn);
                    QString rootId;
                    for (const fs::FsRoot& root : roots) {
                        if (root.id == QStringLiteral("workspace")) {
                            rootId = root.id;
                            break;
                        }
                    }
                    if (rootId.isEmpty() && !roots.isEmpty()) {
                        rootId = roots.first().id;
                    }
                    if (rootId.isEmpty()) {
                        setUploading(false);
                        setStatusText(QString());
                        emit failed(tr("No writable workspace to upload into."));
                        return;
                    }

                    auto writeConn = std::make_shared<QMetaObject::Connection>();
                    *writeConn = connect(
                        fsService, &fs::IFsService::writeResult, this,
                        [this, path, kind,
                         writeConn](const QString& /*rootId*/, const QString& writtenPath, bool ok,
                                    const QString& /*revision*/, const QString& error) {
                            if (writtenPath != path) {
                                return; // a different upload's result; keep waiting for ours
                            }
                            disconnect(*writeConn);
                            setUploading(false);
                            setStatusText(QString());
                            if (ok) {
                                emit uploaded(path, kind);
                            } else {
                                emit failed(error.isEmpty() ? tr("Upload failed.") : error);
                            }
                        });
                    // New file: unconditional write (empty base revision).
                    fsService->write(rootId, path, bytes, QString(), false);
                });
    fsService->listRoots();
}
