// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "app/image_cache.h"

#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QThreadPool>

namespace be::app {

namespace {

// Hard cap on any single decoded dimension; bounds VRAM for oversized images
// when no explicit sourceSize was requested (e.g. rich-text <img>).
constexpr int kMaxDecodeDim = 2048;

QSize cappedTarget(const QSize& source, const QSize& requested) {
    if (source.isEmpty()) {
        return {};
    }

    int cap = kMaxDecodeDim;
    if (requested.width() > 0) {
        cap = qMin(cap, requested.width());
    }
    if (requested.height() > 0) {
        cap = qMin(cap, requested.height());
    }

    const int longest = qMax(source.width(), source.height());
    if (longest <= cap) {
        return source; // no downscale needed (never upscale here)
    }

    const double scale = double(cap) / double(longest);
    return {qMax(1, int(source.width() * scale)), qMax(1, int(source.height() * scale))};
}

} // namespace

ImageCache::ImageCache(QObject* parent) : QObject(parent) {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!base.isEmpty()) {
        auto* diskCache = new QNetworkDiskCache(this);
        diskCache->setCacheDirectory(QDir(base).filePath(QStringLiteral("images")));
        m_network.setCache(diskCache);
    }
}

ImageCache* ImageCache::instance() {
    static auto* cache = new ImageCache();
    return cache;
}

bool ImageCache::tryGet(const QUrl& url, QImage& out) const {
    QMutexLocker lock(&m_mutex);
    const auto it = m_memory.constFind(url);
    if (it == m_memory.constEnd()) {
        return false;
    }
    out = it.value();
    return true;
}

void ImageCache::request(const QUrl& url, const QSize& requestedSize) {
    {
        QMutexLocker lock(&m_mutex);
        if (m_memory.contains(url)) {
            // Already decoded; let waiters resolve via tryGet().
            lock.unlock();
            emit ready(url);
            return;
        }
    }

    if (m_inFlight.contains(url)) {
        return;
    }
    m_inFlight.insert(url);

    const QString scheme = url.scheme().toLower();
    if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) {
        QNetworkRequest req(url);
        req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("qml-block-editor"));
        QNetworkReply* reply = m_network.get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, url, requestedSize]() {
            QByteArray data;
            if (reply->error() == QNetworkReply::NoError) {
                data = reply->readAll();
            }
            reply->deleteLater();
            decodeAsync(url, data, requestedSize);
        });
        return;
    }

    // Local / embedded source: read bytes directly, then decode off-thread.
    decodeAsync(url, readLocalBytes(url), requestedSize);
}

void ImageCache::decodeAsync(const QUrl& url, const QByteArray& data, const QSize& requested) {
    if (data.isEmpty()) {
        finish(url, QImage());
        return;
    }

    QThreadPool::globalInstance()->start([this, url, data, requested]() {
        const QImage image = decodeCapped(data, requested);
        QMetaObject::invokeMethod(
            this, [this, url, image]() { finish(url, image); }, Qt::QueuedConnection);
    });
}

void ImageCache::finish(const QUrl& url, const QImage& image) {
    store(url, image);
    m_inFlight.remove(url);
    emit ready(url);
}

void ImageCache::store(const QUrl& url, const QImage& image) {
    QMutexLocker lock(&m_mutex);
    if (m_memory.contains(url)) {
        m_lru.removeOne(url);
    }
    m_memory.insert(url, image);
    m_lru.append(url);
    while (m_lru.size() > m_maxEntries) {
        const QUrl evicted = m_lru.takeFirst();
        m_memory.remove(evicted);
    }
}

QImage ImageCache::decodeCapped(const QByteArray& data, const QSize& requested) {
    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        return {};
    }

    QImageReader reader(&buffer);
    reader.setAutoTransform(true);

    const QSize source = reader.size();
    const QSize target = cappedTarget(source, requested);
    if (target.isValid() && !target.isEmpty() && target != source) {
        reader.setScaledSize(target);
    }

    return reader.read();
}

QByteArray ImageCache::readLocalBytes(const QUrl& url) {
    const QString scheme = url.scheme().toLower();

    if (scheme == QStringLiteral("data")) {
        // data:[<mediatype>][;base64],<data>
        const QString text = url.toString();
        const qsizetype comma = text.indexOf(QLatin1Char(','));
        if (comma < 0) {
            return {};
        }
        const QString meta = text.left(comma);
        const QByteArray payload = text.mid(comma + 1).toUtf8();
        if (meta.contains(QStringLiteral(";base64"), Qt::CaseInsensitive)) {
            return QByteArray::fromBase64(payload);
        }
        return QByteArray::fromPercentEncoding(payload);
    }

    QString path;
    if (scheme == QStringLiteral("qrc")) {
        path = QStringLiteral(":") + url.path();
    } else if (scheme == QStringLiteral("file")) {
        path = url.toLocalFile();
    } else if (scheme.isEmpty()) {
        path = url.toString();
    } else {
        return {};
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

} // namespace be::app
