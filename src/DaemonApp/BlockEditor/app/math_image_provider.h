#pragma once

#include <QCache>
#include <QImage>
#include <QQuickImageProvider>
#include <QSizeF>
#include <QString>

namespace be::app {

// MicroTeX draws each glyph with a QFont whose point size equals
// Formula::PIXELS_PER_POINT and relies entirely on a painter transform for the
// final size. A 1pt base (the library default) overflows Qt's FreeType raster
// once the transform passes ~20x (HiDPI supersampling alone pushes it there),
// silently dropping every glyph. We instead pin PIXELS_PER_POINT to this value
// via tex::Formula::setDPITarget(72 * kMathBaseFontPt) at startup and render on
// a 72-DPI surface (so 1 size-unit == 1px), then pass textSize = fontPx /
// kMathBaseFontPt to keep the residual transform small. Must match the value
// the host passes to setDPITarget(); the QFonts are cached on first use, so it
// has to be fixed for the process lifetime.
inline constexpr float kMathBaseFontPt = 16.f;

// Rasterizes LaTeX math (served as image://math/<payload>) into QImages via
// MicroTeX's QPainter backend. Qt calls requestImage() off the GUI thread, so
// the whole parse+paint is serialized behind a QMutex: MicroTeX's parser uses
// global macro/symbol state and is not reentrant. Results are memoized in a
// small QCache keyed by the (theme/size-sensitive) request id, so re-rendering
// the same formula is free until a theme or font-size change rotates the id.
//
// tex::LaTeX::init()/release() are driven once by the host (application.cpp);
// this provider only parses, and assumes init has already run.
class MathImageProvider : public QQuickImageProvider {
public:
    MathImageProvider();

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    QImage renderLatex(const QString& latex, bool displayMode, int fontPx, quint32 colorArgb);
    QImage errorImage() const;

    QCache<QString, QImage> m_cache;
};

// Logical (device-independent) size MicroTeX lays a formula out at, matching the
// size the rasterized image reports. The inline projector needs this to give the
// RichText <img> an explicit width/height: Qt's RichText image handler never
// forwards a requested size to the provider, so without it the supersampled
// pixels render oversized. Parses only (no rasterization) and shares MicroTeX's
// non-reentrant parser lock with requestImage(), so it is safe to call from the
// GUI thread while the provider renders on the pixmap thread. Returns an invalid
// QSizeF if parsing fails.
QSizeF measureMathLogicalSize(const QString& latex, bool display, int fontPx);

} // namespace be::app
