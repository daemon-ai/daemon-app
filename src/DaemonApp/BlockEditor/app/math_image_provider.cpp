#include "app/math_image_provider.h"

#include "core/math_url.h"

#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QtMath>

#include <exception>

#include <latex.h>
#include <platform/qt/graphic_qt.h>
#include <render.h>
#include <utils/utf.h>

namespace be::app {

namespace {

// Cap the supersample factor so a formula on a fractional-DPI screen still
// rasterizes crisply without blowing up the cached pixel budget.
qreal renderScale()
{
    qreal dpr = 1.0;
    if (auto *screen = QGuiApplication::primaryScreen()) {
        dpr = screen->devicePixelRatio();
    }
    return qBound(1.0, qCeil(dpr) > 0 ? qreal(qCeil(dpr)) : 1.0, 3.0);
}

} // namespace

MathImageProvider::MathImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    // Bound the memoized rasters; theme/size changes rotate ids, so stale
    // entries age out naturally under the LRU policy.
    m_cache.setMaxCost(256);
}

QImage MathImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    QMutexLocker locker(&m_mutex);

    if (const QImage *cached = m_cache.object(id)) {
        if (size) {
            *size = cached->size() / cached->devicePixelRatio();
        }
        return *cached;
    }

    const be::MathRequest req = be::parseMathImageId(id);
    if (!req.valid) {
        return errorImage();
    }

    const QImage image = renderLatex(req.latex, req.displayMode, req.fontPx, req.colorArgb);
    if (image.isNull()) {
        // A null image surfaces as Image.Error in QML so the block shows its
        // error box; inline math simply drops the broken glyph.
        return image;
    }

    m_cache.insert(id, new QImage(image));
    if (size) {
        *size = image.size() / image.devicePixelRatio();
    }
    return image;
}

QImage MathImageProvider::renderLatex(const QString &latex, bool displayMode, int fontPx, quint32 colorArgb)
{
    Q_UNUSED(displayMode);

    try {
        const std::wstring wide = tex::utf82wide(latex.toStdString());
        // width is a MAX width (MicroTeX renders to the formula's natural width
        // when narrower), so a large value disables wrapping.
        tex::TeXRender *render =
            tex::LaTeX::parse(wide, 100000, static_cast<float>(fontPx), 7.f, static_cast<tex::color>(colorArgb));
        if (render == nullptr) {
            return {};
        }

        const int w = qMax(1, render->getWidth());
        const int h = qMax(1, render->getHeight());
        const qreal scale = renderScale();

        QImage image(QSize(qRound(w * scale), qRound(h * scale)), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setRenderHint(QPainter::TextAntialiasing, true);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter.scale(scale, scale);
            tex::Graphics2D_qt g2(&painter);
            render->draw(g2, 0, 0);
        }
        image.setDevicePixelRatio(scale);

        delete render;
        return image;
    } catch (const std::exception &) {
        return {};
    }
}

QImage MathImageProvider::errorImage() const
{
    return {};
}

} // namespace be::app
