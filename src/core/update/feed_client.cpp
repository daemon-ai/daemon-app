// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/feed_client.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace update {

namespace {

constexpr int kMaxAttempts = 3;
constexpr int kRetryDelayMs = 1500;

bool isLoopbackHost(const QString& host) {
    return host == QStringLiteral("127.0.0.1") || host == QStringLiteral("::1") ||
           host == QStringLiteral("localhost");
}

QNetworkRequest makeRequest(const QUrl& url, const QString& etag) {
    QNetworkRequest req(url);
    // Follow redirects, but never downgrade to a less-safe scheme.
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    if (!etag.isEmpty()) {
        req.setRawHeader(QByteArrayLiteral("If-None-Match"), etag.toLatin1());
    }
    return req;
}

} // namespace

FeedClient::FeedClient(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), m_network(network) {}

bool FeedClient::isAllowedUrl(const QUrl& url) {
    if (!url.isValid()) {
        return false;
    }
    const QString scheme = url.scheme().toLower();
    if (scheme == QStringLiteral("https") || scheme == QStringLiteral("file")) {
        return true;
    }
    // Plaintext HTTP only on loopback (the E2E test harness serves a local feed).
    if (scheme == QStringLiteral("http") && isLoopbackHost(url.host())) {
        return true;
    }
    return false;
}

void FeedClient::fetch(const QUrl& manifestUrl, const QString& etag) {
    if (m_busy) {
        return;
    }
    if (!isAllowedUrl(manifestUrl)) {
        emit failed(QStringLiteral("feed URL scheme is not allowed: %1").arg(manifestUrl.scheme()));
        return;
    }
    m_busy = true;
    m_manifestUrl = manifestUrl;
    m_etag = etag;
    m_attempt = 0;
    m_manifestBytes.clear();
    m_resolvedManifestUrl.clear();
    m_newEtag.clear();
    startManifestRequest();
}

void FeedClient::startManifestRequest() {
    ++m_attempt;
    QNetworkReply* reply = m_network->get(makeRequest(m_manifestUrl, m_etag));
    connect(reply, &QNetworkReply::finished, this, [this, reply] { onManifestFinished(reply); });
}

void FeedClient::onManifestFinished(QNetworkReply* reply) {
    reply->deleteLater();

    const QVariant statusVar = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const int status = statusVar.isValid() ? statusVar.toInt() : 0;

    if (status == 304) {
        finishBusy();
        emit notModified();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        retryOrFail(QStringLiteral("manifest fetch failed: %1").arg(reply->errorString()));
        return;
    }

    m_manifestBytes = reply->readAll();
    // The resolved URL (after redirects) is the base artifact files resolve against.
    m_resolvedManifestUrl = reply->url();
    const QByteArray etagHeader = reply->rawHeader(QByteArrayLiteral("ETag"));
    m_newEtag = QString::fromLatin1(etagHeader);

    startMinisigRequest();
}

void FeedClient::startMinisigRequest() {
    // The detached signature is a sibling of the requested manifest URL (so it
    // follows the same redirect), never the ETag-conditional path.
    const QUrl minisigUrl(m_manifestUrl.toString() + QStringLiteral(".minisig"));
    QNetworkReply* reply = m_network->get(makeRequest(minisigUrl, QString()));
    connect(reply, &QNetworkReply::finished, this, [this, reply] { onMinisigFinished(reply); });
}

void FeedClient::onMinisigFinished(QNetworkReply* reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        retryOrFail(QStringLiteral("signature fetch failed: %1").arg(reply->errorString()));
        return;
    }
    const QByteArray minisigBytes = reply->readAll();
    const QByteArray manifestBytes = m_manifestBytes;
    const QUrl resolved = m_resolvedManifestUrl;
    const QString newEtag = m_newEtag;
    finishBusy();
    emit fetched(manifestBytes, minisigBytes, resolved, newEtag);
}

void FeedClient::retryOrFail(const QString& error) {
    if (m_attempt < kMaxAttempts) {
        // Restart the whole fetch (manifest then minisig) after a short backoff.
        QTimer::singleShot(kRetryDelayMs, this, [this] { startManifestRequest(); });
        return;
    }
    finishBusy();
    emit failed(error);
}

void FeedClient::finishBusy() {
    m_busy = false;
}

} // namespace update
