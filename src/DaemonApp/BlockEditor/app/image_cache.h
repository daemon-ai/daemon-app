// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QHash>
#include <QImage>
#include <QList>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QSize>
#include <QUrl>

namespace be::app {

// Process-wide image fetch + decode cache shared by the standalone Image element
// (via CachedImageProvider) and any other consumer. Lives on the GUI thread;
// remote fetches use a QNetworkAccessManager with an on-disk QNetworkDiskCache,
// and decoding happens off the GUI thread on the global thread pool. Decoded
// results (success or failure) are stored in a small in-memory LRU map so repeat
// requests resolve synchronously. file:/qrc:/data:/local-path sources bypass the
// network and are read directly.
class ImageCache : public QObject {
    Q_OBJECT

public:
    static ImageCache* instance();

    // Thread-safe lookup. Returns true when a decode has completed for `url`
    // (whether it succeeded or failed); `out` carries the image (null on
    // failure). Returns false when the result is not yet known.
    bool tryGet(const QUrl& url, QImage& out) const;

public slots:
    // Kick off a fetch + decode for `url` if not already cached or in flight.
    // `requestedSize` caps the decoded dimensions (0 == use the global cap).
    // Emits ready(url) once the result is available. Must be invoked on the
    // ImageCache's own (GUI) thread; cross-thread callers use a queued call.
    void request(const QUrl& url, const QSize& requestedSize);

signals:
    void ready(const QUrl& url);

private:
    explicit ImageCache(QObject* parent = nullptr);

    void decodeAsync(const QUrl& url, const QByteArray& data, const QSize& requested);
    void finish(const QUrl& url, const QImage& image);
    void store(const QUrl& url, const QImage& image);
    static QImage decodeCapped(const QByteArray& data, const QSize& requested);
    static QByteArray readLocalBytes(const QUrl& url);

    QNetworkAccessManager m_network;

    mutable QMutex m_mutex;       // guards m_memory / m_lru
    QHash<QUrl, QImage> m_memory; // null QImage == decode failed
    QList<QUrl> m_lru;            // most-recent at the back

    QSet<QUrl> m_inFlight; // touched only on the GUI thread

    int m_maxEntries = 256;
};

} // namespace be::app
