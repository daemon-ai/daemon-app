#include "app/block_model.h"

#include "core/image_url.h"
#include "core/markdown_parser.h"
#include "core/markdown_table.h"

#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace be::app {

namespace {

QVariantMap buildTableData(const be::BlockRecord &block, const be::InlineProjector &projector)
{
    const be::TableData table = be::parseTable(block.markdown());
    if (table.columns == 0) {
        return {};
    }

    const auto cellMarkup = [&projector](const QString &raw) -> QString {
        be::BlockRecord cell;
        cell.type = be::BlockType::Paragraph;
        cell.markdownUtf8 = raw.toUtf8();
        return projector.project(cell).displayMarkup;
    };

    QVariantList alignments;
    for (int alignment : table.alignments) {
        alignments.push_back(alignment);
    }

    QVariantList header;
    for (const be::TableCell &cell : table.header) {
        header.push_back(cellMarkup(cell.raw));
    }

    QVariantList rows;
    for (const QVector<be::TableCell> &row : table.rows) {
        QVariantList cells;
        for (const be::TableCell &cell : row) {
            cells.push_back(cellMarkup(cell.raw));
        }
        rows.push_back(cells);
    }

    QVariantMap data;
    data.insert(QStringLiteral("columns"), table.columns);
    data.insert(QStringLiteral("alignments"), alignments);
    data.insert(QStringLiteral("header"), header);
    data.insert(QStringLiteral("rows"), rows);
    return data;
}

// Language token from the block: prefer captured metadata, fall back to a
// leading ``` fence on the first line (covers freshly typed blocks).
QString mermaidLanguageOf(const be::BlockRecord &block)
{
    const QString meta = block.metadata.value(QStringLiteral("fenceLanguage")).toString();
    if (!meta.isEmpty()) {
        return meta;
    }
    const QString md = block.markdown();
    const qsizetype nl = md.indexOf(QLatin1Char('\n'));
    QString first = (nl < 0 ? md : md.left(nl)).trimmed();
    if (!first.startsWith(QStringLiteral("```")) && !first.startsWith(QStringLiteral("~~~"))) {
        return {};
    }
    const QString rest = first.mid(3).trimmed();
    const qsizetype sp = rest.indexOf(QLatin1Char(' '));
    return sp < 0 ? rest : rest.left(sp);
}

// The diagram source for the block: the fenced body with any opening/closing
// fence lines stripped (md4qt already stores the body alone, but a freshly typed
// block keeps its fences, so handle both).
QString mermaidSourceOf(const be::BlockRecord &block)
{
    const QString md = block.markdown();
    QStringList lines = md.split(QLatin1Char('\n'));
    if (!lines.isEmpty()) {
        const QString first = lines.first().trimmed();
        if (first.startsWith(QStringLiteral("```")) || first.startsWith(QStringLiteral("~~~"))) {
            lines.removeFirst();
            if (!lines.isEmpty()) {
                const QString last = lines.last().trimmed();
                if (last.startsWith(QStringLiteral("```")) || last.startsWith(QStringLiteral("~~~"))) {
                    lines.removeLast();
                }
            }
        }
    }
    return lines.join(QLatin1Char('\n'));
}

QVariantMap buildMermaidData(const be::BlockRecord &block)
{
    const QString lang = mermaidLanguageOf(block);
    if (lang.compare(QStringLiteral("mermaid"), Qt::CaseInsensitive) != 0) {
        return {};
    }
    QVariantMap data;
    data.insert(QStringLiteral("language"), QStringLiteral("mermaid"));
    data.insert(QStringLiteral("source"), mermaidSourceOf(block));
    return data;
}

// Image attributes for an Image block: prefer the captured metadata, fall back to
// re-parsing the block's markdown (covers streamed/typed blocks that never went
// through the full parse path). `source` is the QML-ready, scheme-resolved URL.
QVariantMap buildImageData(const be::BlockRecord &block)
{
    be::ImageBlockInfo info;
    info.url = block.metadata.value(QStringLiteral("imageUrl")).toString();
    if (info.url.isEmpty()) {
        if (!be::parseImageBlock(block.markdown(), &info)) {
            return {};
        }
    } else {
        info.alt = block.metadata.value(QStringLiteral("imageAlt")).toString();
        info.title = block.metadata.value(QStringLiteral("imageTitle")).toString();
        info.link = block.metadata.value(QStringLiteral("imageLink")).toString();
        info.width = block.metadata.value(QStringLiteral("imageWidth")).toString();
        info.height = block.metadata.value(QStringLiteral("imageHeight")).toString();
    }

    if (info.url.isEmpty()) {
        return {};
    }

    bool widthIsPercent = false;
    bool heightIsPercent = false;
    const qreal widthValue = be::imageDimensionValue(info.width, &widthIsPercent);
    const qreal heightValue = be::imageDimensionValue(info.height, &heightIsPercent);

    QVariantMap data;
    data.insert(QStringLiteral("url"), info.url);
    data.insert(QStringLiteral("alt"), info.alt);
    data.insert(QStringLiteral("title"), info.title);
    data.insert(QStringLiteral("link"), info.link);
    data.insert(QStringLiteral("source"), be::resolveImageSource(info.url));
    data.insert(QStringLiteral("isRemote"), be::isRemoteImage(info.url));
    data.insert(QStringLiteral("widthValue"), widthValue);
    data.insert(QStringLiteral("widthIsPercent"), widthIsPercent);
    data.insert(QStringLiteral("heightValue"), heightValue);
    data.insert(QStringLiteral("heightIsPercent"), heightIsPercent);
    return data;
}

} // namespace

BlockModel::BlockModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int BlockModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_store) {
        return 0;
    }
    return static_cast<int>(m_store->blockCount());
}

QVariant BlockModel::data(const QModelIndex &index, int role) const
{
    if (!m_store || !index.isValid()) {
        return {};
    }

    const be::BlockRecord *block = m_store->blockAt(index.row());
    if (!block) {
        return {};
    }

    switch (role) {
    case BlockIdRole:
        return QVariant::fromValue<qulonglong>(block->id);
    case TypeRole:
        return static_cast<int>(block->type);
    case TypeNameRole:
        return be::blockTypeName(block->type);
    case MarkdownRole:
        return block->markdown();
    case DisplayMarkupRole:
        return m_projector.project(*block, false, -1, m_contentWidth).displayMarkup;
    case ProjectionRevisionRole:
        return block->renderRevision;
    case HeightHintRole:
        return block->heightHint;
    case DirtyRole:
        return block->dirtyText || block->dirtyRender || block->dirtyPersistence;
    case IndentRole:
        return static_cast<int>(block->indent);
    case TableDataRole:
        return block->type == be::BlockType::Table ? buildTableData(*block, m_projector) : QVariantMap();
    case MermaidDataRole:
        return buildMermaidData(*block);
    case ImageDataRole:
        return block->type == be::BlockType::Image ? buildImageData(*block) : QVariantMap();
    default:
        return {};
    }
}

QHash<int, QByteArray> BlockModel::roleNames() const
{
    return {
        {BlockIdRole, "blockId"},
        {TypeRole, "blockType"},
        {TypeNameRole, "blockTypeName"},
        {MarkdownRole, "markdown"},
        {DisplayMarkupRole, "displayMarkup"},
        {ProjectionRevisionRole, "projectionRevision"},
        {HeightHintRole, "heightHint"},
        {DirtyRole, "dirty"},
        {IndentRole, "indent"},
        {TableDataRole, "tableData"},
        {MermaidDataRole, "mermaidData"},
        {ImageDataRole, "imageData"},
    };
}

void BlockModel::setStore(be::DocumentStore *store)
{
    beginResetModel();
    m_store = store;
    endResetModel();
}

void BlockModel::setContentWidth(qreal width)
{
    // Quantize to whole px so a resize drag emits at most one refresh per pixel.
    const qreal rounded = std::max(0.0, std::floor(width));
    if (qFuzzyCompare(rounded, m_contentWidth)) {
        return;
    }
    m_contentWidth = rounded;
    emit contentWidthChanged();

    // Only the percent-sized inline image markup depends on the column width, so
    // refresh just that role for the visible rows.
    const int rows = rowCount();
    if (rows > 0) {
        emit dataChanged(index(0), index(rows - 1), {DisplayMarkupRole});
    }
}

void BlockModel::setPalette(const be::Palette &palette)
{
    m_projector.setPalette(palette);
    const int rows = rowCount();
    if (rows > 0) {
        emit dataChanged(index(0), index(rows - 1), {DisplayMarkupRole, TableDataRole});
    }
}

void BlockModel::resetFromStore()
{
    beginResetModel();
    endResetModel();
}

void BlockModel::notifyBlockChanged(be::BlockId id)
{
    if (!m_store) {
        return;
    }
    const qsizetype row = m_store->rowForBlock(id);
    if (row < 0) {
        resetFromStore();
        return;
    }
    const QModelIndex modelIndex = index(static_cast<int>(row));
    emit dataChanged(modelIndex, modelIndex);
}

void BlockModel::applyChangeSet(const be::BlockChangeSet &changeSet)
{
    if (!m_store) {
        return;
    }

    // The store has already mutated its block list; emit precise structural and
    // dataChanged signals so the view updates incrementally (no beginResetModel,
    // so scroll position, delegate reuse, and any active editor are preserved).
    // Inserts/removes happen at structuralRow; the dataChanged range covers the
    // overlapping volatile rows below it, so their indices are never disturbed.
    if (changeSet.removedCount > 0) {
        const int first = static_cast<int>(changeSet.structuralRow);
        beginRemoveRows({}, first, first + static_cast<int>(changeSet.removedCount) - 1);
        endRemoveRows();
    }
    if (changeSet.insertedCount > 0) {
        const int first = static_cast<int>(changeSet.structuralRow);
        beginInsertRows({}, first, first + static_cast<int>(changeSet.insertedCount) - 1);
        endInsertRows();
    }
    if (changeSet.changedFirst >= 0) {
        const QModelIndex top = index(static_cast<int>(changeSet.changedFirst));
        const QModelIndex bottom = index(static_cast<int>(changeSet.changedLast));
        emit dataChanged(top, bottom);
    }
}

be::BlockId BlockModel::blockIdAt(int row) const
{
    if (!m_store) {
        return 0;
    }
    const be::BlockRecord *block = m_store->blockAt(row);
    return block ? block->id : 0;
}

} // namespace be::app
