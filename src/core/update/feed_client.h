// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
QT_END_NAMESPACE

namespace update {

// Fetches the signed feed: manifest.json plus its detached manifest.json.minisig.
//
// - HTTPS-or-file transport only (file:// and http on loopback are allowed for
//   tests; every other plaintext scheme is refused). The signature - not the
//   transport - authenticates the feed, but HTTPS is kept as defense in depth.
// - Follows redirects (GitHub's releases/latest/download/... 302s to a versioned
//   asset URL); the resolved manifest URL is reported so artifact `file` fields
//   resolve relative to it.
// - Sends If-None-Match with the stored ETag and reports a 304 as up-to-date;
//   stores the new ETag from a 200.
// - Bounded retries on transient network failure.
//
// The client does NO verification or parsing - it only transports the two blobs.
// UpdateManager verifies the signature, then parses.
class FeedClient : public QObject {
    Q_OBJECT

public:
    FeedClient(QNetworkAccessManager* network, QObject* parent = nullptr);

    // Begin a fetch of `manifestUrl` (+ "<manifestUrl>.minisig"). `etag` is the
    // last stored validator (empty = unconditional). Only one fetch runs at a
    // time; a new call while busy is ignored.
    void fetch(const QUrl& manifestUrl, const QString& etag);

    // True while a fetch is in flight.
    [[nodiscard]] bool busy() const { return m_busy; }

    // Scheme allow-list gate (https, file, or http on loopback). Static so the
    // manager can pre-check a runtime feed-URL override before dispatching.
    [[nodiscard]] static bool isAllowedUrl(const QUrl& url);

signals:
    // The manifest was unchanged since `etag` (HTTP 304).
    void notModified();
    // Both blobs fetched. `resolvedManifestUrl` is the manifest URL after
    // redirects (the base for resolving artifact files); `newEtag` may be empty.
    void fetched(const QByteArray& manifestBytes, const QByteArray& minisigBytes,
                 const QUrl& resolvedManifestUrl, const QString& newEtag);
    // The fetch failed after exhausting retries.
    void failed(const QString& error);

private:
    void startManifestRequest();
    void onManifestFinished(QNetworkReply* reply);
    void startMinisigRequest();
    void onMinisigFinished(QNetworkReply* reply);
    void retryOrFail(const QString& error);
    void finishBusy();

    QNetworkAccessManager* m_network;
    bool m_busy = false;
    QUrl m_manifestUrl;
    QString m_etag;
    int m_attempt = 0;

    // Carried between the manifest and minisig legs of one fetch.
    QByteArray m_manifestBytes;
    QUrl m_resolvedManifestUrl;
    QString m_newEtag;
};

} // namespace update
