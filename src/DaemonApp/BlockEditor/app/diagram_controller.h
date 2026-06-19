#pragma once

#include "diagram/engine/diagram_engine.h"
#include "diagram/snapshot/render_snapshot.h"

#include <QColor>
#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>

#include <atomic>

namespace be::app {

// Owns the diagram source/theme inputs and produces an immutable RenderSnapshot.
// The build runs on a worker thread (latest-wins); results are published back to
// the GUI thread. DiagramItem reads the published snapshot in updatePaintNode.
class DiagramController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal contentWidth READ contentWidth WRITE setContentWidth NOTIFY contentWidthChanged)
    Q_PROPERTY(qreal fontPixelSize READ fontPixelSize WRITE setFontPixelSize NOTIFY styleChanged)
    Q_PROPERTY(QColor surfaceColor READ surfaceColor WRITE setSurfaceColor NOTIFY styleChanged)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY styleChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY styleChanged)
    Q_PROPERTY(QColor edgeColor READ edgeColor WRITE setEdgeColor NOTIFY styleChanged)
    Q_PROPERTY(QColor clusterColor READ clusterColor WRITE setClusterColor NOTIFY styleChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY snapshotChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY snapshotChanged)
    Q_PROPERTY(qreal diagramWidth READ diagramWidth NOTIFY snapshotChanged)
    Q_PROPERTY(qreal diagramHeight READ diagramHeight NOTIFY snapshotChanged)

public:
    explicit DiagramController(QObject *parent = nullptr);
    ~DiagramController() override;

    QString source() const { return m_source; }
    void setSource(const QString &source);

    qreal contentWidth() const { return m_contentWidth; }
    void setContentWidth(qreal width);

    qreal fontPixelSize() const { return m_fontPixelSize; }
    void setFontPixelSize(qreal size);

    QColor surfaceColor() const { return m_style.nodeFill; }
    void setSurfaceColor(const QColor &c);
    QColor borderColor() const { return m_style.nodeStroke; }
    void setBorderColor(const QColor &c);
    QColor textColor() const { return m_style.textColor; }
    void setTextColor(const QColor &c);
    QColor edgeColor() const { return m_style.edgeStroke; }
    void setEdgeColor(const QColor &c);
    QColor clusterColor() const { return m_style.clusterFill; }
    void setClusterColor(const QColor &c);

    bool hasError() const;
    QString errorText() const;
    qreal diagramWidth() const;
    qreal diagramHeight() const;

    // Non-QML accessor used by DiagramItem on the render thread.
    be::diagram::RenderSnapshotPtr snapshot() const { return m_snapshot; }

signals:
    void sourceChanged();
    void contentWidthChanged();
    void styleChanged();
    void snapshotChanged();
    void buildRequested(const QString &source, be::diagram::Style style, qreal maxWidth,
                        quint64 requestId);

private:
    void scheduleRebuild();
    void startBuild();
    void publish(be::diagram::RenderSnapshotPtr snapshot, quint64 requestId);

    QString m_source;
    qreal m_contentWidth = 0.0;
    qreal m_fontPixelSize = 13.0;
    be::diagram::Style m_style;

    be::diagram::RenderSnapshotPtr m_snapshot;
    QTimer m_debounce;

    QThread m_worker;
    std::atomic<quint64> m_requestId{0};
};

} // namespace be::app
