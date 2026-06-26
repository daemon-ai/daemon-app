#include "app/cached_image_provider.h"

#include "app/image_cache.h"

#include <QQuickTextureFactory>
#include <QUrl>

namespace be::app {

namespace {

// One in-flight request. Created on the QQuickPixmap reader thread (which runs an
// event loop), so the queued ImageCache::ready signal is delivered here safely.
class CachedImageResponse : public QQuickImageResponse {
public:
    CachedImageResponse(const QString& id, const QSize& requestedSize)
        : m_url(QUrl::fromPercentEncoding(id.toUtf8())) {
        ImageCache* cache = ImageCache::instance();

        if (cache->tryGet(m_url, m_image)) {
            emit finished();
            return;
        }

        connect(cache, &ImageCache::ready, this, &CachedImageResponse::onReady);

        // Re-check in case the decode completed between tryGet() and connect().
        if (cache->tryGet(m_url, m_image)) {
            emit finished();
            return;
        }

        QMetaObject::invokeMethod(cache, "request", Qt::QueuedConnection, Q_ARG(QUrl, m_url),
                                  Q_ARG(QSize, requestedSize));
    }

    [[nodiscard]] QQuickTextureFactory* textureFactory() const override {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    [[nodiscard]] QString errorString() const override {
        return m_image.isNull() ? QStringLiteral("image load failed") : QString();
    }

private:
    void onReady(const QUrl& url) {
        if (url != m_url || m_done) {
            return;
        }
        ImageCache::instance()->tryGet(m_url, m_image);
        m_done = true;
        emit finished();
    }

    QUrl m_url;
    QImage m_image;
    bool m_done = false;
};

} // namespace

QQuickImageResponse* CachedImageProvider::requestImageResponse(const QString& id,
                                                               const QSize& requestedSize) {
    return new CachedImageResponse(id, requestedSize);
}

} // namespace be::app
