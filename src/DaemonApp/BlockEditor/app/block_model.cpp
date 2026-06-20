#include "app/block_model.h"

#include "app/math_image_provider.h"
#include "core/agent_block.h"
#include "core/image_url.h"
#include "core/markdown_parser.h"
#include "core/markdown_table.h"
#include "core/math_url.h"

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

// Block-level math payload: the LaTeX source rendered as a centered display
// formula. Detected from EITHER a ```math fence (reusing the fence helpers) OR a
// block whose entire content is a single $$...$$ span. `source` is the bare
// LaTeX; the QML layer turns it into an image://math URL with the live palette.
QVariantMap buildMathData(const be::BlockRecord &block)
{
    QString source;

    // An already-parsed math fence keeps its language in metadata and stores the
    // body alone, so trust the metadata directly. Otherwise (freshly typed fence
    // or a bare $$...$$ block) detect from the raw markdown.
    const QString lang = mermaidLanguageOf(block);
    if (lang.compare(QStringLiteral("math"), Qt::CaseInsensitive) == 0) {
        source = mermaidSourceOf(block).trimmed();
    } else {
        source = be::blockMathSource(block.markdown());
    }

    if (source.isEmpty()) {
        return {};
    }

    QVariantMap data;
    data.insert(QStringLiteral("source"), source);
    data.insert(QStringLiteral("displayMode"), true);
    return data;
}

// Syntax-highlight payload for a code fence: the info-string language (for the
// KSyntaxHighlighting definition lookup) and the fenced body with the fence
// lines stripped. Mermaid fences are excluded - they render as diagrams via
// buildMermaidData, not as highlighted source.
QVariantMap buildCodeData(const be::BlockRecord &block)
{
    if (block.type != be::BlockType::CodeFence) {
        return {};
    }
    const QString lang = mermaidLanguageOf(block);
    // Mermaid and math fences render as their own block types, not code cards.
    if (lang.compare(QStringLiteral("mermaid"), Qt::CaseInsensitive) == 0
        || lang.compare(QStringLiteral("math"), Qt::CaseInsensitive) == 0) {
        return {};
    }
    QVariantMap data;
    data.insert(QStringLiteral("language"), lang);
    data.insert(QStringLiteral("code"), mermaidSourceOf(block));
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

// Tool-call view model for a ToolCall block: the structured metadata shaped into
// display fields (title flip, status, duration, detail kind) by the shared core
// transform, ready for ToolCallBlock.qml + its sub-renderers.
QVariantMap buildToolData(const be::BlockRecord &block)
{
    if (block.type != be::BlockType::ToolCall) {
        return {};
    }
    return be::buildToolView(block.metadata);
}

// Reasoning view model: the scalar fields (status/duration) plus the chain-of-
// thought body rendered to display markup via the projector, so ReasoningBlock
// shows real markdown (mirrors buildTableData projecting its cells).
QVariantMap buildReasoningData(const be::BlockRecord &block, const be::InlineProjector &projector)
{
    if (block.type != be::BlockType::Reasoning) {
        return {};
    }
    QVariantMap view = be::buildReasoningView(block.metadata);
    const QString body = view.value(QStringLiteral("body")).toString();
    if (!body.isEmpty()) {
        be::BlockRecord para;
        para.type = be::BlockType::Paragraph;
        para.markdownUtf8 = body.toUtf8();
        view.insert(QStringLiteral("displayMarkup"), projector.project(para).displayMarkup);
    }
    return view;
}

QVariantMap buildContentData(const be::BlockRecord &block)
{
    if (block.type != be::BlockType::Content) {
        return {};
    }
    return be::buildContentView(block.metadata);
}

} // namespace

BlockModel::BlockModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // Inline math needs an explicit logical size on its <img> (RichText won't
    // ask the provider for one). Delegate to MicroTeX's measurer here so core
    // stays free of any rendering dependency.
    m_projector.setMathMeasurer([](const QString &latex, bool display, int fontPx) {
        return be::app::measureMathLogicalSize(latex, display, fontPx);
    });
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
    case CodeDataRole:
        return buildCodeData(*block);
    case ImageDataRole:
        return block->type == be::BlockType::Image ? buildImageData(*block) : QVariantMap();
    case MathDataRole:
        return buildMathData(*block);
    case ToolDataRole:
        return buildToolData(*block);
    case ReasoningDataRole:
        return buildReasoningData(*block, m_projector);
    case ContentDataRole:
        return buildContentData(*block);
    case MessageRoleRole:
        return be::messageRoleToString(block->role);
    case MessageIdRole:
        return block->messageId;
    case MessageFirstRole: {
        if (block->role == be::MessageRole::None) {
            return false;
        }
        const be::BlockRecord *prev = m_store->blockAt(index.row() - 1);
        return !prev || prev->role != block->role || prev->messageId != block->messageId;
    }
    case MessageLastRole: {
        if (block->role == be::MessageRole::None) {
            return false;
        }
        const be::BlockRecord *next = m_store->blockAt(index.row() + 1);
        return !next || next->role != block->role || next->messageId != block->messageId;
    }
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
        {CodeDataRole, "codeData"},
        {ImageDataRole, "imageData"},
        {MathDataRole, "mathData"},
        {ToolDataRole, "toolData"},
        {ReasoningDataRole, "reasoningData"},
        {ContentDataRole, "contentData"},
        {MessageRoleRole, "messageRole"},
        {MessageIdRole, "messageId"},
        {MessageFirstRole, "messageFirst"},
        {MessageLastRole, "messageLast"},
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

    // A structural change can flip the run-edge flags of the rows bracketing the
    // splice (the block above an insert is no longer "last", etc.). Run-edges are
    // cheap to recompute, so refresh them across the whole list to stay correct
    // without a full reset (preserving scroll/active state).
    if (changeSet.insertedCount > 0 || changeSet.removedCount > 0) {
        const int rows = rowCount();
        if (rows > 0) {
            emit dataChanged(index(0), index(rows - 1), {MessageFirstRole, MessageLastRole});
        }
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
