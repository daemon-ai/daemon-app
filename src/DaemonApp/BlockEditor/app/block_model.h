#pragma once

#include "core/document_store.h"
#include "core/inline_projector.h"

#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>

namespace be::app {

class BlockModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("BlockModel is owned by EditorController")

    // The flow column width (px) used to resolve percent-sized inline images.
    Q_PROPERTY(
        qreal contentWidth READ contentWidth WRITE setContentWidth NOTIFY contentWidthChanged)

public:
    enum Role {
        BlockIdRole = Qt::UserRole + 1,
        TypeRole,
        TypeNameRole,
        MarkdownRole,
        DisplayMarkupRole,
        ProjectionRevisionRole,
        HeightHintRole,
        DirtyRole,
        IndentRole,
        TableDataRole,
        MermaidDataRole,
        CodeDataRole,
        ImageDataRole,
        MathDataRole,
        ToolDataRole,
        ReasoningDataRole,
        ContentDataRole,
        // Message/role layer (Strategy C): the block's session role, its
        // stable message id, and run-edge flags (first/last block of its
        // contiguous message run) the QML grouping layer uses for bubbles,
        // footers, and inter-turn spacing.
        MessageRoleRole,
        MessageIdRole,
        MessageFirstRole,
        MessageLastRole,
    };
    Q_ENUM(Role)

    explicit BlockModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setStore(be::DocumentStore* store);
    void resetFromStore();
    void notifyBlockChanged(be::BlockId id);
    void applyChangeSet(const be::BlockChangeSet& changeSet);
    be::BlockId blockIdAt(int row) const;

    // Recolors the rendered RichText for every block (theme switch). The display
    // markup is read live from the role, so refreshing it re-renders in place.
    void setPalette(const be::Palette& palette);

    qreal contentWidth() const { return m_contentWidth; }
    void setContentWidth(qreal width);

signals:
    void contentWidthChanged();

private:
    be::DocumentStore* m_store = nullptr;
    be::InlineProjector m_projector;
    qreal m_contentWidth = 0;
};

} // namespace be::app
