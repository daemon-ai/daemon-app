// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/update_downloader.h"

#include "update/semver.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

namespace update {

namespace {

constexpr qint64 kStalePartAgeSecs = 7 * 24 * 60 * 60; // 7 days

QString updatesRoot() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(base).filePath(QStringLiteral("updates"));
}

// A version dir name -> version string is 1:1 (we name dirs by the raw version).
bool makeDir0700(const QString& path) {
    QDir dir;
    if (!dir.mkpath(path)) {
        return false;
    }
    // Owner-only rwx (best effort; on platforms without POSIX perms this is a no-op).
    QFile::setPermissions(path,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    return true;
}

} // namespace

UpdateDownloader::UpdateDownloader(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), m_network(network) {}

UpdateDownloader::~UpdateDownloader() {
    cleanupReply();
    delete m_partFile;
}

QString UpdateDownloader::stagingDir(const QString& version) {
    return QDir(updatesRoot()).filePath(version);
}

void UpdateDownloader::start(const QUrl& artifactUrl, const QString& version,
                             const QString& fileName, qint64 expectedSize,
                             const QString& sha256Hex) {
    if (m_busy) {
        return;
    }
    if (fileName.isEmpty() || expectedSize <= 0 || sha256Hex.isEmpty()) {
        emit failed(QStringLiteral("download requires a file name, size, and sha256"));
        return;
    }
    const QString dir = stagingDir(version);
    if (!makeDir0700(dir)) {
        emit failed(QStringLiteral("cannot create staging directory"));
        return;
    }

    m_busy = true;
    m_url = artifactUrl;
    m_version = version;
    m_expectedSize = expectedSize;
    m_sha256 = sha256Hex.toLower();
    m_finalPath = QDir(dir).filePath(fileName);
    // Deterministic .part name (per-artifact, inside the 0700 per-version dir) so a
    // partial download can be resumed across restarts; promoted by atomic rename.
    m_partPath = m_finalPath + QStringLiteral(".part");

    // If the final verified file already exists, skip straight to promotion.
    if (QFileInfo::exists(m_finalPath)) {
        m_partPath = m_finalPath; // verify the finished file in place
        if (verifyAndPromote()) {
            const QString path = m_finalPath;
            m_busy = false;
            emit progress(1.0);
            emit finished(path);
            return;
        }
        // A stale/corrupt final file: drop it and re-download.
        QFile::remove(m_finalPath);
        m_partPath = m_finalPath + QStringLiteral(".part");
    }

    // Resume if a partial exists and is shorter than the expected size.
    m_resumeFrom = 0;
    const QFileInfo partInfo(m_partPath);
    if (partInfo.exists()) {
        if (partInfo.size() >= m_expectedSize) {
            QFile::remove(m_partPath); // oversized/complete partial: start clean
        } else {
            m_resumeFrom = partInfo.size();
        }
    }

    m_partFile = new QFile(m_partPath, this);
    const QIODevice::OpenMode mode = m_resumeFrom > 0
                                         ? (QIODevice::Append | QIODevice::WriteOnly)
                                         : (QIODevice::Truncate | QIODevice::WriteOnly);
    if (!m_partFile->open(mode)) {
        delete m_partFile;
        m_partFile = nullptr;
        m_busy = false;
        emit failed(QStringLiteral("cannot open download temp file"));
        return;
    }

    QNetworkRequest req(m_url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    if (m_resumeFrom > 0) {
        req.setRawHeader(QByteArrayLiteral("Range"), QByteArrayLiteral("bytes=") +
                                                         QByteArray::number(m_resumeFrom) +
                                                         QByteArrayLiteral("-"));
    }
    m_reply = m_network->get(req);
    connect(m_reply, &QNetworkReply::readyRead, this, &UpdateDownloader::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &UpdateDownloader::onFinished);
}

void UpdateDownloader::onReadyRead() {
    if (m_reply == nullptr || m_partFile == nullptr) {
        return;
    }
    const QByteArray chunk = m_reply->readAll();
    if (m_partFile->write(chunk) != chunk.size()) {
        failWith(QStringLiteral("write to staging file failed"));
        return;
    }
    // Size pre-gate: abort early if the running total already exceeds the signed
    // size (a mismatched or hostile artifact) rather than downloading it all.
    const qint64 written = m_partFile->size();
    if (written > m_expectedSize) {
        failWith(QStringLiteral("download exceeds the expected size"));
        return;
    }
    if (m_expectedSize > 0) {
        emit progress(static_cast<double>(written) / static_cast<double>(m_expectedSize));
    }
}

void UpdateDownloader::onFinished() {
    if (m_reply == nullptr) {
        return;
    }
    const QNetworkReply::NetworkError err = m_reply->error();
    const QString errString = m_reply->errorString();
    cleanupReply();

    if (m_partFile != nullptr) {
        m_partFile->flush();
        m_partFile->close();
    }

    if (err != QNetworkReply::NoError) {
        // Keep the .part for a later resume; report the failure.
        delete m_partFile;
        m_partFile = nullptr;
        m_busy = false;
        emit failed(QStringLiteral("download failed: %1").arg(errString));
        return;
    }

    delete m_partFile;
    m_partFile = nullptr;

    // Size pre-gate on the completed file.
    if (QFileInfo(m_partPath).size() != m_expectedSize) {
        QFile::remove(m_partPath);
        m_busy = false;
        emit failed(QStringLiteral("downloaded size does not match the manifest"));
        return;
    }

    if (!verifyAndPromote()) {
        QFile::remove(m_partPath);
        m_busy = false;
        emit failed(QStringLiteral("sha256 does not match the manifest"));
        return;
    }

    const QString path = m_finalPath;
    m_busy = false;
    emit progress(1.0);
    emit finished(path);
}

bool UpdateDownloader::verifyAndPromote() {
    QFile file(m_partPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        return false;
    }
    file.close();
    const QString digest = QString::fromLatin1(hash.result().toHex());
    if (digest != m_sha256) {
        return false;
    }
    if (m_partPath == m_finalPath) {
        return true; // already the final verified file
    }
    QFile::remove(m_finalPath); // rename() will not overwrite on all platforms
    return QFile::rename(m_partPath, m_finalPath);
}

void UpdateDownloader::cancel() {
    if (!m_busy) {
        return;
    }
    cleanupReply();
    if (m_partFile != nullptr) {
        m_partFile->flush();
        m_partFile->close();
        delete m_partFile;
        m_partFile = nullptr;
    }
    m_busy = false;
}

void UpdateDownloader::failWith(const QString& error) {
    cleanupReply();
    if (m_partFile != nullptr) {
        m_partFile->close();
        delete m_partFile;
        m_partFile = nullptr;
    }
    m_busy = false;
    emit failed(error);
}

void UpdateDownloader::cleanupReply() {
    if (m_reply != nullptr) {
        m_reply->disconnect(this);
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void UpdateDownloader::cleanupStale(const QString& currentVersion) {
    QDir root(updatesRoot());
    if (!root.exists()) {
        return;
    }
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QList<QFileInfo> versionDirs = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& dirInfo : versionDirs) {
        const QString version = dirInfo.fileName();
        // Drop version dirs already installed or superseded (v <= current).
        if (!currentVersion.isEmpty() && semver::compare(version, currentVersion) <= 0) {
            QDir(dirInfo.absoluteFilePath()).removeRecursively();
            continue;
        }
        // Otherwise, sweep stale .part temps older than the retention window.
        QDir versionDir(dirInfo.absoluteFilePath());
        const QList<QFileInfo> parts =
            versionDir.entryInfoList({QStringLiteral("*.part")}, QDir::Files);
        for (const QFileInfo& part : parts) {
            if (part.lastModified().secsTo(now) > kStalePartAgeSecs) {
                QFile::remove(part.absoluteFilePath());
            }
        }
    }
}

} // namespace update
