#include "app/diagram_controller.h"

#include <QFont>

namespace be::app {

namespace {

// Worker object that runs the (pure) engine off the GUI thread. Lives on the
// controller's worker thread; results are delivered via a queued signal.
class DiagramWorker : public QObject {
    Q_OBJECT
public:
    using Style = be::diagram::Style;
    using SnapshotPtr = be::diagram::RenderSnapshotPtr;

    void build(const QString& source, const be::diagram::Style& style, qreal maxWidth,
               quint64 requestId) {
        be::diagram::DiagramEngine engine;
        SnapshotPtr snap = engine.buildSnapshot(source, style, maxWidth, requestId);
        emit produced(snap, requestId);
    }

signals:
    void produced(be::diagram::RenderSnapshotPtr _t1, quint64 _t2);
};

} // namespace

DiagramController::DiagramController(QObject* parent) : QObject(parent) {
    qRegisterMetaType<be::diagram::RenderSnapshotPtr>("be::diagram::RenderSnapshotPtr");
    qRegisterMetaType<be::diagram::Style>("be::diagram::Style");

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(0);
    connect(&m_debounce, &QTimer::timeout, this, &DiagramController::startBuild);

    auto* worker = new DiagramWorker;
    worker->moveToThread(&m_worker);
    connect(&m_worker, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &DiagramController::buildRequested, worker, &DiagramWorker::build);
    connect(worker, &DiagramWorker::produced, this,
            [this](be::diagram::RenderSnapshotPtr snapshot, quint64 requestId) {
                publish(std::move(snapshot), requestId);
            });
    m_worker.start();
}

DiagramController::~DiagramController() {
    m_worker.quit();
    m_worker.wait();
}

void DiagramController::setSource(const QString& source) {
    if (m_source == source) {
        return;
    }
    m_source = source;
    emit sourceChanged();
    scheduleRebuild();
}

void DiagramController::setContentWidth(qreal width) {
    if (qFuzzyCompare(m_contentWidth, width)) {
        return;
    }
    m_contentWidth = width;
    emit contentWidthChanged();
    scheduleRebuild();
}

void DiagramController::setFontPixelSize(qreal size) {
    if (qFuzzyCompare(m_fontPixelSize, size)) {
        return;
    }
    m_fontPixelSize = size;
    emit styleChanged();
    scheduleRebuild();
}

void DiagramController::setSurfaceColor(const QColor& c) {
    if (m_style.nodeFill == c) {
        return;
    }
    m_style.nodeFill = c;
    emit styleChanged();
    scheduleRebuild();
}

void DiagramController::setBorderColor(const QColor& c) {
    if (m_style.nodeStroke == c) {
        return;
    }
    m_style.nodeStroke = c;
    m_style.clusterStroke = c;
    emit styleChanged();
    scheduleRebuild();
}

void DiagramController::setTextColor(const QColor& c) {
    if (m_style.textColor == c) {
        return;
    }
    m_style.textColor = c;
    emit styleChanged();
    scheduleRebuild();
}

void DiagramController::setEdgeColor(const QColor& c) {
    if (m_style.edgeStroke == c) {
        return;
    }
    m_style.edgeStroke = c;
    emit styleChanged();
    scheduleRebuild();
}

void DiagramController::setClusterColor(const QColor& c) {
    if (m_style.clusterFill == c) {
        return;
    }
    m_style.clusterFill = c;
    m_style.labelBackground = c;
    emit styleChanged();
    scheduleRebuild();
}

bool DiagramController::hasError() const {
    return m_snapshot && m_snapshot->hasError;
}

QString DiagramController::errorText() const {
    return m_snapshot ? m_snapshot->errorText : QString();
}

qreal DiagramController::diagramWidth() const {
    return m_snapshot ? m_snapshot->diagramBounds.width() : 0.0;
}

qreal DiagramController::diagramHeight() const {
    return m_snapshot ? m_snapshot->diagramBounds.height() : 0.0;
}

void DiagramController::scheduleRebuild() {
    m_debounce.start();
}

void DiagramController::startBuild() {
    if (m_source.trimmed().isEmpty()) {
        publish(nullptr, m_requestId.fetch_add(1) + 1);
        return;
    }
    be::diagram::Style style = m_style;
    style.font.setPixelSize(qMax<int>(6, int(m_fontPixelSize)));
    const qreal maxWidth = m_contentWidth > 1.0 ? m_contentWidth : 600.0;
    const quint64 id = m_requestId.fetch_add(1) + 1;
    emit buildRequested(m_source, style, maxWidth, id);
}

void DiagramController::publish(be::diagram::RenderSnapshotPtr snapshot, quint64 requestId) {
    // Latest-wins: drop results from superseded requests.
    if (requestId < m_requestId.load()) {
        return;
    }
    m_snapshot = std::move(snapshot);
    emit snapshotChanged();
}

} // namespace be::app

#include "diagram_controller.moc"
