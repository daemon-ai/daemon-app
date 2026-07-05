// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QFile;
QT_END_NAMESPACE

namespace update {

// Downloads a verified-manifest artifact into a private per-version staging dir
// with the full integrity chain from packaging/UPDATES.md:
//
//   <AppDataLocation>/updates/<version>/<file>
//
// - the staging dir is created 0700;
// - bytes stream to a "<file>.part" temp, resumed via HTTP Range if a partial
//   from a prior run is present and still shorter than the expected size;
// - a size pre-gate aborts early on a byte-count mismatch, then a full sha256
//   gate must match the signed manifest BEFORE the file is exposed;
// - only then is the temp atomically renamed to the final path.
//
// Nothing downstream ever sees a half-written or unverified file.
class UpdateDownloader : public QObject {
    Q_OBJECT

public:
    UpdateDownloader(QNetworkAccessManager* network, QObject* parent = nullptr);
    ~UpdateDownloader() override;

    // Begin (or resume) a download. `artifactUrl` is the fully-resolved artifact
    // URL, `version`/`fileName` name the staging path, `expectedSize` and
    // `sha256Hex` are the signed gates. Ignored if a download is already running.
    void start(const QUrl& artifactUrl, const QString& version, const QString& fileName,
               qint64 expectedSize, const QString& sha256Hex);

    // Abort an in-flight download (keeps the .part for a later resume).
    void cancel();

    [[nodiscard]] bool busy() const { return m_busy; }

    // The staging directory for a version (<AppDataLocation>/updates/<version>).
    [[nodiscard]] static QString stagingDir(const QString& version);

    // Startup hygiene: delete ".part" temps older than 7 days and any
    // updates/<v> directories whose version is <= currentVersion (already
    // installed / superseded). Best-effort; never throws.
    static void cleanupStale(const QString& currentVersion);

signals:
    void progress(double fraction); // 0..1
    void finished(const QString& filePath);
    void failed(const QString& error);

private:
    void cleanupReply();
    void onReadyRead();
    void onFinished();
    void failWith(const QString& error);
    [[nodiscard]] bool verifyAndPromote();

    QNetworkAccessManager* m_network;
    QNetworkReply* m_reply = nullptr;
    QFile* m_partFile = nullptr;

    bool m_busy = false;
    QUrl m_url;
    QString m_version;
    QString m_finalPath;
    QString m_partPath;
    qint64 m_expectedSize = 0;
    QString m_sha256;
    qint64 m_resumeFrom = 0;
};

} // namespace update
