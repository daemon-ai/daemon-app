// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "app/math_image_provider.h"

#include "core/math_url.h"

#include <exception>
#include <latex.h>
#include <platform/qt/graphic_qt.h>
#include <QGuiApplication>
#include <QHash>
#include <QMutex>
#include <QPainter>
#include <QScreen>
#include <QtMath>
#include <render.h>
#include <utils/utf.h>

namespace be::app {

namespace {

// MicroTeX's parser uses non-reentrant global macro/symbol state, so every parse
// (rasterizing on the pixmap thread, measuring on the GUI thread) serializes on
// this one lock.
QMutex& microtexMutex() {
    static QMutex mutex;
    return mutex;
}

// Supersample at the screen's true device pixel ratio so the image maps 1:1 to
// physical pixels. The old qCeil() rounded 1.5x up to 2x, then the compositor
// scaled back down to 1.5x and softened the glyphs. Clamped so an exotic DPR
// can't blow up the cached pixel budget.
qreal renderScale() {
    qreal dpr = 1.0;
    if (auto* screen = QGuiApplication::primaryScreen()) {
        dpr = screen->devicePixelRatio();
    }
    return qBound(1.0, dpr > 0.0 ? dpr : 1.0, 3.0);
}

} // namespace

MathImageProvider::MathImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {
    // Bound the memoized rasters; theme/size changes rotate ids, so stale
    // entries age out naturally under the LRU policy.
    m_cache.setMaxCost(256);
}

QImage MathImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    Q_UNUSED(requestedSize);

    QMutexLocker locker(&microtexMutex());

    if (const QImage* cached = m_cache.object(id)) {
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

QImage MathImageProvider::renderLatex(const QString& latex, bool displayMode, int fontPx,
                                      quint32 colorArgb) {
    Q_UNUSED(displayMode);

    try {
        const std::wstring wide = tex::utf82wide(latex.toStdString());
        // MicroTeX's base font is pinned to kMathBaseFontPt (see the header), so
        // textSize is the multiplier that brings it to the requested fontPx. The
        // width is a MAX width (MicroTeX renders to the formula's natural width
        // when narrower), so a large value disables wrapping.
        const float textSize = static_cast<float>(fontPx) / kMathBaseFontPt;
        tex::TeXRender* render =
            tex::LaTeX::parse(wide, 100000, textSize, 7.f, static_cast<tex::color>(colorArgb));
        if (render == nullptr) {
            return {};
        }

        const int w = qMax(1, render->getWidth());
        const int h = qMax(1, render->getHeight());
        const qreal scale = renderScale();

        // Ceil (not round) the supersampled extent: rounding down can shave a
        // sub-pixel off a thin glyph like a superscript, clipping it at small
        // font sizes (the crop vanishes as the font grows). Ceiling guarantees
        // the box fully contains the formula's ink.
        QImage image(QSize(qCeil(w * scale), qCeil(h * scale)),
                     QImage::Format_ARGB32_Premultiplied);
        // MicroTeX maps 1 size-unit to PIXELS_PER_POINT pixels; rendering on a
        // 72-DPI surface makes a point equal a pixel, so glyph rasterization and
        // the layout metrics agree. With the base font pinned, the only painter
        // magnification is `scale` (HiDPI supersampling), which stays small
        // enough for Qt's FreeType raster.
        const int dpm72 = qRound(72.0 / 0.0254);
        image.setDotsPerMeterX(dpm72);
        image.setDotsPerMeterY(dpm72);
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
    } catch (const std::exception&) {
        return {};
    }
}

QImage MathImageProvider::errorImage() const {
    return {};
}

QSizeF measureMathLogicalSize(const QString& latex, bool display, int fontPx) {
    Q_UNUSED(display);

    // Projection re-runs on every edit, so memoize the (cheap but non-trivial)
    // parse. display is part of the key for symmetry with the image id even
    // though the size is currently display-independent.
    const QString key = QStringLiteral("%1|%2|%3").arg(latex).arg(display ? 1 : 0).arg(fontPx);

    QMutexLocker locker(&microtexMutex());

    static QHash<QString, QSizeF> cache;
    if (const auto it = cache.constFind(key); it != cache.constEnd()) {
        return *it;
    }

    QSizeF result;
    try {
        const std::wstring wide = tex::utf82wide(latex.toStdString());
        // Same parse parameters as renderLatex() (color does not affect layout),
        // so the logical size matches the rasterized image exactly.
        const float textSize = static_cast<float>(fontPx) / kMathBaseFontPt;
        tex::TeXRender* render = tex::LaTeX::parse(wide, 100000, textSize, 7.f, 0x00000000);
        if (render != nullptr) {
            result = QSizeF(qMax(1, render->getWidth()), qMax(1, render->getHeight()));
            delete render;
        }
    } catch (const std::exception&) {
        result = QSizeF();
    }

    // Bound memory: drop the whole table rather than implement LRU; a theme or
    // font-size change rotates every key anyway.
    if (cache.size() >= 512) {
        cache.clear();
    }
    cache.insert(key, result);
    return result;
}

} // namespace be::app
