#pragma once

#include <QCache>
#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

namespace be::app {

// Rasterizes LaTeX math (served as image://math/<payload>) into QImages via
// MicroTeX's QPainter backend. Qt calls requestImage() off the GUI thread, so
// the whole parse+paint is serialized behind a QMutex: MicroTeX's parser uses
// global macro/symbol state and is not reentrant. Results are memoized in a
// small QCache keyed by the (theme/size-sensitive) request id, so re-rendering
// the same formula is free until a theme or font-size change rotates the id.
//
// tex::LaTeX::init()/release() are driven once by the host (application.cpp);
// this provider only parses, and assumes init has already run.
class MathImageProvider : public QQuickImageProvider
{
public:
    MathImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QImage renderLatex(const QString &latex, bool displayMode, int fontPx, quint32 colorArgb);
    QImage errorImage() const;

    QMutex m_mutex;
    QCache<QString, QImage> m_cache;
};

} // namespace be::app
