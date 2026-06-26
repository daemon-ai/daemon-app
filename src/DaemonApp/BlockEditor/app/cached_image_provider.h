#pragma once

#include <QQuickAsyncImageProvider>

namespace be::app {

// Routes image://imgcache/<percent-encoded-url> requests through the shared
// ImageCache. The async response resolves immediately when the image is already
// decoded, otherwise it waits for ImageCache::ready. Returns a GPU texture
// factory so the scene graph uploads the decoded image directly.
class CachedImageProvider : public QQuickAsyncImageProvider {
public:
    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize& requestedSize) override;
};

} // namespace be::app
