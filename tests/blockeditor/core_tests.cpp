#include "core/coordinate_map.h"
#include "core/block_height_index.h"
#include "core/command_stack.h"
#include "core/document_store.h"
#include "core/image_url.h"
#include "core/inline_projector.h"
#include "core/markdown_parser.h"
#include "core/markdown_table.h"
#include "core/persistence.h"
#include "core/piece_table.h"
#include "core/selection.h"

#include <QTemporaryDir>
#include <QtTest>

class CoreTests : public QObject
{
    Q_OBJECT

private slots:
    void parserSmoke();
    void coordinateMapRoundTrip_data();
    void coordinateMapRoundTrip();
    void pieceTableReplace();
    void documentStoreRoundTrip();
    void codeFenceRoundTrips_data();
    void codeFenceRoundTrips();
    void projectionMapsMarkdown();
    void projectionOffsetEndpointsRoundTrip();
    void projectionRendersLinks();
    void linkSelectionCopiesFullSyntax();
    void linkNegativeCasesNotLinks();
    void linkInsideEmphasisIsClickable();
    void headingAnchorResolvesToRow();
    void imageBlocksClassify();
    void inlineImagesProject();
    void resolveImageSourceCases();
    void imageSizingParses();
    void selectionCopiesMarkdown();
    void selectionSpanExposesRawOffsets();
    void persistenceSavesDirtyBlocks();
    void blockHeightIndexFindsRows();
    void commandStackUndoRedo();
    void listFlattensPerItem();
    void splitKeepsBlocksSeparate();
    void splitPreservesBlockIdentity();
    void splitMergeRoundTrips();
    void splitUsesUtf16CursorOffset();
    void listItemSplitContinuesMarker();
    void emptyListItemSplitExitsList();
    void orderedListRenumbersOnSplit();
    void headingSplitTailBecomesParagraph();
    void codeFenceEnterExitsToNewParagraph();
    void codeFenceEnterInBodyStaysInFence();
    void streamMatchesOneShotLoad();
    void streamCommitsBlockIdentity();
    void streamSetextReclassifies();
    void streamRoundTripThenStructuralEdit();
    void streamUndoRemovesAppendedBlocks();
    void multilineParagraphRoundTrips();
    void paragraphBreakTrimsBlankLine();
    void paragraphBreakTrimsTrailingSpaces();
    void paragraphBreakPreservesInternalSoftBreak();
    void nestedListRoundTrips();
    void indentRespectsMaxDepth();
    void outdentFloorsAtZero();
    void backspacePolicyOutdentUnlistMerge();
    void nestedOrderedRenumbering();
    void mixedListRunDepthAndSeparators();
    void tableClassifiedFromContent();
    void tableRoundTrips();
    void tableParseStructure();
    void tableParsePipesInCodeAndEscapes();
    void tableCellRawOffsets();
    void tableSurvivesEdit();
    void tableSplitCreatesTrailingParagraph();
    void pasteHeadingDocIntoHeadingNoLeadingNewline();
    void pasteSplitsMidParagraph();
    void pasteSingleLineMergesInline();
};

namespace {

// Feed a markdown string into a streaming session in fixed-size chunks, mimicking
// token-by-token arrival (no coalescing timer at the store level).
void streamInChunks(be::DocumentStore &store, const QString &markdown, int chunkSize)
{
    store.beginStreamAtEnd();
    for (qsizetype i = 0; i < markdown.size(); i += chunkSize) {
        store.streamAppend(markdown.mid(i, chunkSize));
    }
    store.endStream();
}

} // namespace

void CoreTests::parserSmoke()
{
    be::MarkdownParser parser;
    const be::ParsedMarkdown parsed = parser.parse(QStringLiteral("# Title\n\nA **bold** paragraph.\n"));
    QVERIFY(parsed.document);
    QVERIFY(parsed.blocks.size() >= 2);
    QCOMPARE(parsed.blocks.first().type, be::BlockType::Heading);
    QCOMPARE(parsed.blocks.first().headingLevel, quint16(1));
    QCOMPARE(parsed.blocks.first().startLine, qsizetype(0));
}

void CoreTests::coordinateMapRoundTrip_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("ascii") << QStringLiteral("alpha\nbeta");
    QTest::newRow("emoji") << QString::fromUtf8("a 😀 b\nc");
    QTest::newRow("combining") << QString::fromUtf8("Cafe\u0301\ntext");
    QTest::newRow("cjk") << QString::fromUtf8("行一\n行二");
    QTest::newRow("rtl") << QString::fromUtf8("אבג\ntext");
}

void CoreTests::coordinateMapRoundTrip()
{
    QFETCH(QString, text);

    be::CoordinateMap map;
    map.rebuild(text);

    for (qsizetype utf16 = 0; utf16 <= text.size(); ++utf16) {
        const qsizetype utf8 = map.utf16ToUtf8(utf16);
        QVERIFY(utf8 >= 0);
        QVERIFY(utf8 <= text.toUtf8().size());
        QVERIFY(map.utf8ToUtf16(utf8) <= utf16);

        const be::LineColumn lc = map.utf16ToLineColumn(utf16);
        QCOMPARE(map.lineColumnToUtf16(lc.line, lc.column), utf16);
    }
}

void CoreTests::pieceTableReplace()
{
    be::PieceTable table;
    table.setOriginal(QByteArrayLiteral("hello world"));
    table.replace(6, 5, QByteArrayLiteral("markdown"));
    QCOMPARE(table.toUtf8(), QByteArrayLiteral("hello markdown"));
    table.replace(0, 5, QByteArrayLiteral("hi"));
    QCOMPARE(table.toUtf8(), QByteArrayLiteral("hi markdown"));
}

void CoreTests::documentStoreRoundTrip()
{
    const QString markdown = QStringLiteral("# Title\n\nParagraph\n\n- item\n");
    be::DocumentStore store;
    store.loadMarkdown(markdown);
    QCOMPARE(store.toMarkdown(), markdown);
    QVERIFY(store.blockCount() >= 3);
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Heading);
}

void CoreTests::codeFenceRoundTrips_data()
{
    QTest::addColumn<QString>("markdown");

    QTest::newRow("code") << QStringLiteral("```javascript\ncode\n```\n");
    QTest::newRow("empty body") << QStringLiteral("```javascript\n\n```\n");
    QTest::newRow("blank line in body") << QStringLiteral("```\ncode\n\n```\n");
    QTest::newRow("mermaid") << QStringLiteral("```mermaid\ngraph TD;\nA-->B;\n```\n");
    QTest::newRow("tilde") << QStringLiteral("~~~python\nprint(1)\n~~~\n");
    QTest::newRow("around paragraphs")
        << QStringLiteral("Intro\n\n```javascript\ncode\n```\n\nOutro\n");
}

void CoreTests::codeFenceRoundTrips()
{
    QFETCH(QString, markdown);

    be::DocumentStore store;
    store.loadMarkdown(markdown);

    // The fenced block must survive a load -> serialize round-trip intact: this
    // is the path the plaintext view exercises when toggling render modes.
    QCOMPARE(store.toMarkdown(), markdown);

    // Find the code fence block and confirm its stored markdown keeps both the
    // opening and closing delimiters (md4qt drops them from its span).
    const be::BlockRecord *fence = nullptr;
    for (qsizetype row = 0; row < store.blockCount(); ++row) {
        if (store.blockAt(row)->type == be::BlockType::CodeFence) {
            fence = store.blockAt(row);
            break;
        }
    }
    QVERIFY(fence != nullptr);
    const QString fenceMarkdown = fence->markdown();
    const bool opensBacktick = fenceMarkdown.startsWith(QStringLiteral("```"));
    const bool opensTilde = fenceMarkdown.startsWith(QStringLiteral("~~~"));
    QVERIFY(opensBacktick || opensTilde);
    QVERIFY(fenceMarkdown.trimmed().endsWith(opensTilde ? QStringLiteral("~~~")
                                                        : QStringLiteral("```")));
}

void CoreTests::projectionMapsMarkdown()
{
    be::BlockRecord block;
    block.id = 1;
    block.type = be::BlockType::Paragraph;
    block.markdownUtf8 = QByteArrayLiteral("A **bold** word");

    be::InlineProjector projector;
    const be::BlockProjection projection = projector.project(block);
    QCOMPARE(projection.visualText, QStringLiteral("A bold word"));
    QCOMPARE(projection.visualToRaw(2), qsizetype(4));
    QCOMPARE(projection.rawToVisual(4), qsizetype(2));
}

void CoreTests::projectionOffsetEndpointsRoundTrip()
{
    struct Case {
        const char *name;
        be::BlockType type;
        quint16 headingLevel;
        QByteArray markdown;
    };

    const Case cases[] = {
        {"strong", be::BlockType::Paragraph, 0, QByteArrayLiteral("A **bold** word")},
        {"emphasis", be::BlockType::Paragraph, 0, QByteArrayLiteral("An _emphasised_ word")},
        {"code", be::BlockType::Paragraph, 0, QByteArrayLiteral("Use `code` here")},
        {"heading", be::BlockType::Heading, 2, QByteArrayLiteral("## Title text")},
        {"bullet", be::BlockType::BulletListItem, 0, QByteArrayLiteral("- list item")},
        {"task", be::BlockType::TaskListItem, 0, QByteArrayLiteral("- [ ] do the thing")},
        {"task-done", be::BlockType::TaskListItem, 0, QByteArrayLiteral("- [x] done thing")},
        {"softbreak", be::BlockType::Paragraph, 0, QByteArrayLiteral("first line\nsecond line")},
        {"link", be::BlockType::Paragraph, 0, QByteArrayLiteral("see [Qt](https://qt.io) docs")},
        {"autolink", be::BlockType::Paragraph, 0, QByteArrayLiteral("ping <https://qt.io> now")},
    };

    be::InlineProjector projector;
    for (const Case &c : cases) {
        be::BlockRecord block;
        block.id = 1;
        block.type = c.type;
        block.headingLevel = c.headingLevel;
        block.markdownUtf8 = c.markdown;

        const be::BlockProjection projection = projector.project(block);
        const qsizetype rawSize = block.markdown().size();
        const qsizetype visualSize = projection.visualText.size();

        // Endpoints must map to endpoints in both directions. Presentation text
        // (heading prefixes, bullet/task markers) is injected with zero-length
        // raw affinity, which is exactly where endpoint mapping tends to break.
        QVERIFY2(projection.rawToVisual(rawSize) == visualSize, c.name);
        QVERIFY2(projection.visualToRaw(visualSize) == rawSize, c.name);
    }
}

void CoreTests::projectionRendersLinks()
{
    be::InlineProjector projector;

    {
        be::BlockRecord block;
        block.id = 1;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("See [Qt](https://qt.io) docs");
        const be::BlockProjection p = projector.project(block);
        QCOMPARE(p.visualText, QStringLiteral("See Qt docs"));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<a href=\"https://qt.io\"")));
        QVERIFY(p.displayMarkup.contains(QStringLiteral(">Qt</span></a>")));
    }

    {
        be::BlockRecord block;
        block.id = 2;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("ping <https://qt.io> now");
        const be::BlockProjection p = projector.project(block);
        QCOMPARE(p.visualText, QStringLiteral("ping https://qt.io now"));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<a href=\"https://qt.io\"")));
    }

    {
        // Query parameters (and the '&' that html-escapes) survive intact.
        be::BlockRecord block;
        block.id = 3;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("[a](https://x.com/p?q=1&r=2)");
        const be::BlockProjection p = projector.project(block);
        QCOMPARE(p.visualText, QStringLiteral("a"));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<a href=\"https://x.com/p?q=1&amp;r=2\"")));
    }

    {
        // A link whose label contains nested images is one link; the images
        // reduce to their (empty) alt text and the outer url is the href.
        be::BlockRecord block;
        block.id = 4;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral(
            "[![](https://e/f?domain=a&sz=128)GitHub+3![](https://e/f?domain=b)Ruby+3](https://rubymamistvalove.com/block-editor)");
        const be::BlockProjection p = projector.project(block);
        QCOMPARE(p.visualText, QStringLiteral("GitHub+3Ruby+3"));
        QCOMPARE(p.displayMarkup.count(QStringLiteral("<a href")), qsizetype(1));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<a href=\"https://rubymamistvalove.com/block-editor\"")));
        QVERIFY(!p.displayMarkup.contains(QStringLiteral("![")));
    }

    {
        // Emphasis inside a link label reduces to its inner text.
        be::BlockRecord block;
        block.id = 5;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("[**bold** x](https://x)");
        const be::BlockProjection p = projector.project(block);
        QCOMPARE(p.visualText, QStringLiteral("bold x"));
        QCOMPARE(p.displayMarkup.count(QStringLiteral("<a href")), qsizetype(1));
    }

    {
        // An image-only label collapses to empty, so the url is shown as the
        // clickable text instead of an invisible link.
        be::BlockRecord block;
        block.id = 6;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("[![](https://i)](https://x)");
        const be::BlockProjection p = projector.project(block);
        QCOMPARE(p.visualText, QStringLiteral("https://x"));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<a href=\"https://x\"")));
    }
}

void CoreTests::linkInsideEmphasisIsClickable()
{
    be::InlineProjector projector;

    // **[text](url)**: the link is wrapped in bold, but must still render as a
    // single clickable <a> (bold), not literal "[text](url)".
    {
        be::BlockRecord block;
        block.id = 1;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("**[text](https://x)**");
        const be::BlockProjection p = projector.project(block);
        QCOMPARE(p.visualText, QStringLiteral("text"));
        QCOMPARE(p.displayMarkup.count(QStringLiteral("<a href=\"https://x\"")), qsizetype(1));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<b>")));
        QVERIFY(!p.displayMarkup.contains(QStringLiteral("[text]")));
    }

    // *[t](u)*: italic variant stays clickable.
    {
        be::BlockRecord block;
        block.id = 2;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("*[t](https://y)*");
        const be::BlockProjection p = projector.project(block);
        QCOMPARE(p.visualText, QStringLiteral("t"));
        QCOMPARE(p.displayMarkup.count(QStringLiteral("<a href=\"https://y\"")), qsizetype(1));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<i>")));
    }

    // The real-world "Back to Top" TOC pattern: the bold anchor preserves the
    // #fragment href so anchor navigation can resolve it.
    {
        be::BlockRecord block;
        block.id = 3;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("**[\xE2\xAC\x86 Back to Top](#table-of-contents)**");
        const be::BlockProjection p = projector.project(block);
        QCOMPARE(p.displayMarkup.count(QStringLiteral("<a href=\"#table-of-contents\"")), qsizetype(1));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<b>")));
    }
}

void CoreTests::headingAnchorResolvesToRow()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("### Table of Contents\n\nbody\n\n## Foo Bar!\n"));

    // GitHub-style slug match, with the leading '#' optional.
    QCOMPARE(store.rowForHeadingAnchor(QStringLiteral("#table-of-contents")), qsizetype(0));
    QCOMPARE(store.rowForHeadingAnchor(QStringLiteral("table-of-contents")), qsizetype(0));
    // Trailing punctuation ('!') is dropped from the slug.
    QCOMPARE(store.rowForHeadingAnchor(QStringLiteral("#foo-bar")), qsizetype(2));
    // No matching heading.
    QCOMPARE(store.rowForHeadingAnchor(QStringLiteral("#nope")), qsizetype(-1));
}

void CoreTests::linkSelectionCopiesFullSyntax()
{
    be::InlineProjector projector;

    // Selecting the rendered label of a link-only block copies the full Markdown
    // link, not just the label, because the link span's raw range is the whole
    // [label](url).
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("[Qt](https://qt.io)\n"));
        QVERIFY(store.blockCount() >= 1);
        const be::BlockRecord *block = store.blockAt(0);
        const be::BlockProjection p = projector.project(*block);
        QCOMPARE(p.visualText, QStringLiteral("Qt"));

        be::SelectionControllerCore selection;
        selection.setAnchor({block->id, 0, p.visualToRaw(0), 0});
        selection.setHead({block->id, 0, p.visualToRaw(p.visualText.size()), p.visualText.size()});
        QCOMPARE(selection.copyMarkdown(store.blocks()), QStringLiteral("[Qt](https://qt.io)"));
    }

    // A link embedded in text: selecting the whole block preserves the URL.
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("See [Qt](https://qt.io) docs\n"));
        const be::BlockRecord *block = store.blockAt(0);
        const be::BlockProjection p = projector.project(*block);

        be::SelectionControllerCore selection;
        selection.setAnchor({block->id, 0, p.visualToRaw(0), 0});
        selection.setHead({block->id, 0, p.visualToRaw(p.visualText.size()), p.visualText.size()});
        QCOMPARE(selection.copyMarkdown(store.blocks()), QStringLiteral("See [Qt](https://qt.io) docs"));
    }

    // Cross-block selection fully containing the link block preserves the URL.
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("first\n\n[Qt](https://qt.io)\n\nthird\n"));
        QVERIFY(store.blockCount() >= 3);
        const be::BlockRecord *b0 = store.blockAt(0);
        const be::BlockRecord *b2 = store.blockAt(2);

        be::SelectionControllerCore selection;
        selection.setAnchor({b0->id, 0, 2, 2});
        selection.setHead({b2->id, 2, 3, 3});
        QVERIFY(selection.copyMarkdown(store.blocks()).contains(QStringLiteral("[Qt](https://qt.io)")));
    }
}

void CoreTests::linkNegativeCasesNotLinks()
{
    be::InlineProjector projector;

    struct Case {
        const char *name;
        QByteArray markdown;
        QString expectedVisual;
    };

    const Case cases[] = {
        {"empty-url", QByteArrayLiteral("[x]()"), QStringLiteral("[x]()")},
        {"space-url", QByteArrayLiteral("[x](a b)"), QStringLiteral("[x](a b)")},
    };

    for (const Case &c : cases) {
        be::BlockRecord block;
        block.id = 1;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = c.markdown;
        const be::BlockProjection p = projector.project(block);
        QVERIFY2(!p.displayMarkup.contains(QStringLiteral("<a href")), c.name);
        bool hasLinkSpan = false;
        for (const be::InlineSpan &span : p.spans) {
            if (span.kind == be::SpanKind::Link) {
                hasLinkSpan = true;
            }
        }
        QVERIFY2(!hasLinkSpan, c.name);
        QCOMPARE(p.visualText, c.expectedVisual);
    }
}

void CoreTests::imageBlocksClassify()
{
    // A line that is solely an image becomes an Image block with url/alt captured.
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("![logo](https://x/a.png)\n"));
        QVERIFY(store.blockCount() >= 1);
        const be::BlockRecord *block = store.blockAt(0);
        QCOMPARE(block->type, be::BlockType::Image);
        QCOMPARE(block->metadata.value(QStringLiteral("imageUrl")).toString(),
                 QStringLiteral("https://x/a.png"));
        QCOMPARE(block->metadata.value(QStringLiteral("imageAlt")).toString(),
                 QStringLiteral("logo"));
        QVERIFY(block->metadata.value(QStringLiteral("imageLink")).toString().isEmpty());
    }

    // A linked standalone image captures the click-through page as imageLink.
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("[![alt](https://i/p.png)](https://page)\n"));
        QVERIFY(store.blockCount() >= 1);
        const be::BlockRecord *block = store.blockAt(0);
        QCOMPARE(block->type, be::BlockType::Image);
        QCOMPARE(block->metadata.value(QStringLiteral("imageUrl")).toString(),
                 QStringLiteral("https://i/p.png"));
        QCOMPARE(block->metadata.value(QStringLiteral("imageLink")).toString(),
                 QStringLiteral("https://page"));
    }

    // A favicon-in-link paragraph (image + visible label) stays a Paragraph so it
    // can render inline rather than as a standalone image block.
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("[![](https://f/icon.png)Docs](https://page)\n"));
        QVERIFY(store.blockCount() >= 1);
        const be::BlockRecord *block = store.blockAt(0);
        QCOMPARE(block->type, be::BlockType::Paragraph);
    }

    // Live edits reclassify through classifyContent + buildImageData fallback.
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("hello\n"));
        const be::BlockRecord *block = store.blockAt(0);
        store.replaceBlockMarkdown(block->id, QStringLiteral("![pic](https://y/b.png)"));
        const be::BlockRecord *updated = store.blockAt(0);
        QCOMPARE(updated->type, be::BlockType::Image);
        QCOMPARE(updated->metadata.value(QStringLiteral("imageUrl")).toString(),
                 QStringLiteral("https://y/b.png"));
    }
}

void CoreTests::inlineImagesProject()
{
    be::InlineProjector projector;

    // Favicon-in-link: the paragraph stays inline; its display markup carries both
    // the provider-routed favicon <img> and the link anchor.
    {
        be::BlockRecord block;
        block.id = 1;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("[![](https://f/icon.png)Docs](https://page)");
        const be::BlockProjection p = projector.project(block);
        QCOMPARE(p.visualText, QStringLiteral("Docs"));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<img src=\"image://imgcache")));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<a href=\"https://page\"")));
    }

    // A bare inline image among words becomes one Image span (a single placeholder
    // in the visual text) and an <img> in the display markup.
    {
        be::BlockRecord block;
        block.id = 2;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("see ![alt](https://x/a.png) here");
        const be::BlockProjection p = projector.project(block);
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<img src=\"image://imgcache")));
        QVERIFY(!p.displayMarkup.contains(QStringLiteral("<a href")));
        int imageSpans = 0;
        for (const be::InlineSpan &span : p.spans) {
            if (span.kind == be::SpanKind::Image) {
                ++imageSpans;
            }
        }
        QCOMPARE(imageSpans, 1);
    }
}

void CoreTests::resolveImageSourceCases()
{
    // Remote -> provider URL with the original percent-encoded as the id.
    QVERIFY(be::isRemoteImage(QStringLiteral("https://x/a.png")));
    QVERIFY(be::resolveImageSource(QStringLiteral("https://x/a.png?q=1&s=2"))
                .startsWith(QStringLiteral("image://imgcache/")));

    // Local schemes pass through untouched and are not "remote".
    QVERIFY(!be::isRemoteImage(QStringLiteral("file:///tmp/a.png")));
    QCOMPARE(be::resolveImageSource(QStringLiteral("file:///tmp/a.png")),
             QStringLiteral("file:///tmp/a.png"));
    QCOMPARE(be::resolveImageSource(QStringLiteral("data:image/png;base64,AAAA")),
             QStringLiteral("data:image/png;base64,AAAA"));

    // A bare absolute path becomes a file:// URL (loaded directly, never cached).
    QVERIFY(be::resolveImageSource(QStringLiteral("/tmp/a.png"))
                .startsWith(QStringLiteral("file://")));
}

void CoreTests::imageSizingParses()
{

    // Dimension parsing: units, percent, and physical conversions.
    {
        bool pct = false;
        QCOMPARE(be::imageDimensionValue(QStringLiteral("200"), &pct), 200.0);
        QVERIFY(!pct);
        QCOMPARE(be::imageDimensionValue(QStringLiteral("200px"), &pct), 200.0);
        QVERIFY(!pct);
        QCOMPARE(be::imageDimensionValue(QStringLiteral("50%"), &pct), 50.0);
        QVERIFY(pct);
        QCOMPARE(qRound(be::imageDimensionValue(QStringLiteral("1in"), &pct)), 96);
        QVERIFY(!pct);
        QCOMPARE(be::imageDimensionValue(QString(), &pct), 0.0);
    }

    // Attribute extraction from a Pandoc attribute block.
    {
        const QString attrs = QStringLiteral("{ width=300 height=20px }");
        QCOMPARE(be::imageAttribute(attrs, QStringLiteral("width")), QStringLiteral("300"));
        QCOMPARE(be::imageAttribute(attrs, QStringLiteral("height")), QStringLiteral("20px"));
        QVERIFY(be::imageAttribute(attrs, QStringLiteral("class")).isEmpty());
    }

    // parseImageBlock captures the trailing attribute block.
    {
        be::ImageBlockInfo info;
        QVERIFY(be::parseImageBlock(QStringLiteral("![Tux](https://x/tux.png){ width=300 }"), &info));
        QCOMPARE(info.url, QStringLiteral("https://x/tux.png"));
        QCOMPARE(info.width, QStringLiteral("300"));
        QVERIFY(info.height.isEmpty());
    }

    // A sized standalone image is an Image block; the size lands in metadata.
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("![Tux](https://x/tux.png){ width=50% }\n"));
        QVERIFY(store.blockCount() >= 1);
        const be::BlockRecord *block = store.blockAt(0);
        QCOMPARE(block->type, be::BlockType::Image);
        QCOMPARE(block->metadata.value(QStringLiteral("imageWidth")).toString(),
                 QStringLiteral("50%"));
    }

    // An inline image with an explicit pixel width emits a width attribute.
    {
        be::InlineProjector projector;
        be::BlockRecord block;
        block.id = 1;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("icon ![a](https://x/a.png){width=24} here");
        const be::BlockProjection p = projector.project(block);
        QVERIFY(p.displayMarkup.contains(QStringLiteral("<img src=\"image://imgcache")));
        QVERIFY(p.displayMarkup.contains(QStringLiteral("width=\"24\"")));
    }

    // An inline image with a percent width resolves against the content width
    // (the rich-text engine cannot size a percent <img> itself). Without a width
    // basis the percent is dropped so the image is not forced to natural size.
    {
        be::InlineProjector projector;
        be::BlockRecord block;
        block.id = 2;
        block.type = be::BlockType::Paragraph;
        block.markdownUtf8 = QByteArrayLiteral("icon ![a](https://x/a.png){ width=10% } here");

        const be::BlockProjection sized = projector.project(block, false, -1, 800.0);
        QVERIFY(sized.displayMarkup.contains(QStringLiteral("width=\"80\"")));

        const be::BlockProjection unsized = projector.project(block);
        QVERIFY(!unsized.displayMarkup.contains(QStringLiteral("width=")));
    }
}

void CoreTests::selectionCopiesMarkdown()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("alpha\nbeta\n"));
    QVERIFY(store.blockCount() >= 1);

    be::SelectionControllerCore selection;
    const be::BlockRecord *first = store.blockAt(0);
    selection.setAnchor({first->id, 0, 0, 0});
    selection.setHead({first->id, 0, 5, 5});
    QCOMPARE(selection.copyMarkdown(store.blocks()), QStringLiteral("alpha"));
}

void CoreTests::selectionSpanExposesRawOffsets()
{
    // Three blocks; the middle one carries markdown markers so raw != visual.
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("first\n\nuse **bold** word\n\nthird\n"));
    QVERIFY(store.blockCount() >= 3);
    const be::BlockRecord *b0 = store.blockAt(0);
    const be::BlockRecord *b1 = store.blockAt(1);
    const be::BlockRecord *b2 = store.blockAt(2);

    be::InlineProjector projector;
    const be::BlockProjection p1 = projector.project(*b1);
    const qsizetype raw1 = b1->markdown().size();
    const qsizetype vis1 = p1.visualText.size();
    QVERIFY(raw1 != vis1); // "use **bold** word" vs "use bold word"

    be::SelectionControllerCore selection;
    // Selection from middle of b0 to middle of b2, spanning the whole b1.
    selection.setAnchor({b0->id, 0, 2, 2});
    selection.setHead({b2->id, 2, 3, 3});

    // Middle block: fully covered -> raw span is 0..rawLength (not visualLength).
    const be::SelectionSpan mid = selection.spanForBlock(b1->id, 1, vis1, raw1);
    QCOMPARE(mid.kind, be::SelectionSpan::Kind::FullBlock);
    QCOMPARE(mid.rawStart, qsizetype(0));
    QCOMPARE(mid.rawEnd, raw1);
    QCOMPARE(mid.visualEnd, vis1);

    // Start block: partial from the anchor raw offset to the block's raw end.
    const be::SelectionSpan startSpan = selection.spanForBlock(b0->id, 0, p1.visualText.size(), b0->markdown().size());
    QCOMPARE(startSpan.kind, be::SelectionSpan::Kind::Partial);
    QCOMPARE(startSpan.rawStart, qsizetype(2));
    QCOMPARE(startSpan.rawEnd, b0->markdown().size());

    // End block: partial from 0 to the head raw offset.
    const be::SelectionSpan endSpan = selection.spanForBlock(b2->id, 2, 99, b2->markdown().size());
    QCOMPARE(endSpan.kind, be::SelectionSpan::Kind::Partial);
    QCOMPARE(endSpan.rawStart, qsizetype(0));
    QCOMPARE(endSpan.rawEnd, qsizetype(3));
}

void CoreTests::persistenceSavesDirtyBlocks()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("# Persisted\n"));

    be::ChangedBlockStore persistence;
    QVERIFY(persistence.open(dir.filePath(QStringLiteral("doc.sqlite"))));
    QVERIFY(persistence.saveBlocks(store.blocks()));
    const QVector<be::BlockRecord> blocks = persistence.loadBlocks();
    QCOMPARE(blocks.size(), store.blockCount());
    QCOMPARE(blocks.first().markdown(), store.blockAt(0)->markdown());
}

void CoreTests::blockHeightIndexFindsRows()
{
    be::BlockHeightIndex index;
    index.reset(3, 10.0);
    index.setHeight(1, 25.0);
    QCOMPARE(index.prefixHeight(0), 0.0);
    QCOMPARE(index.prefixHeight(2), 35.0);
    QCOMPARE(index.rowAtContentY(0.0), qsizetype(0));
    QCOMPARE(index.rowAtContentY(10.0), qsizetype(1));
    QCOMPARE(index.rowAtContentY(34.0), qsizetype(1));
    QCOMPARE(index.rowAtContentY(35.0), qsizetype(2));
}

void CoreTests::commandStackUndoRedo()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("alpha\n"));
    const be::BlockId id = store.blockAt(0)->id;
    const QString before = store.blockMarkdown(id);

    be::CommandStack commands;
    commands.push(std::make_unique<be::ReplaceBlockCommand>(id, before, QStringLiteral("beta")), store);
    QCOMPARE(store.toMarkdown(), QStringLiteral("beta\n"));
    QVERIFY(commands.canUndo());

    commands.undo(store);
    QCOMPARE(store.toMarkdown(), QStringLiteral("alpha\n"));
    QVERIFY(commands.canRedo());

    commands.redo(store);
    QCOMPARE(store.toMarkdown(), QStringLiteral("beta\n"));
}

void CoreTests::listFlattensPerItem()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("- a\n- b\n- c\n"));
    QCOMPARE(store.blockCount(), qsizetype(3));
    for (qsizetype row = 0; row < 3; ++row) {
        QCOMPARE(store.blockAt(row)->type, be::BlockType::BulletListItem);
    }
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("- a"));
    QCOMPARE(store.toMarkdown(), QStringLiteral("- a\n- b\n- c\n"));
}

void CoreTests::splitKeepsBlocksSeparate()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("Para\n"));
    const be::BlockId id = store.blockAt(0)->id;

    QVERIFY(store.splitBlock(id, 2));
    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("Pa"));
    QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("ra"));
    // A blank-line separator keeps the halves as two paragraphs (no soft-break merge)
    // and no leaked whitespace.
    QCOMPARE(store.toMarkdown(), QStringLiteral("Pa\n\nra\n"));
}

void CoreTests::splitPreservesBlockIdentity()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("# Title\n\nParagraph\n\n- item\n"));
    const be::BlockId headingId = store.blockAt(0)->id;
    const be::BlockId paragraphId = store.blockAt(1)->id;
    const be::BlockId listId = store.blockAt(2)->id;

    QVERIFY(store.splitBlock(paragraphId, 4));
    QCOMPARE(store.blockCount(), qsizetype(4));

    QCOMPARE(store.blockAt(0)->id, headingId);
    QCOMPARE(store.blockAt(1)->id, paragraphId);
    const be::BlockId insertedId = store.blockAt(2)->id;
    QVERIFY(insertedId != headingId);
    QVERIFY(insertedId != paragraphId);
    QVERIFY(insertedId != listId);
    // The block after the split point keeps its original id (no positional remap).
    QCOMPARE(store.blockAt(3)->id, listId);
}

void CoreTests::splitMergeRoundTrips()
{
    be::DocumentStore store;
    const QString original = QStringLiteral("# Title\n\nParagraph\n\n- item\n");
    store.loadMarkdown(original);
    const be::BlockId paragraphId = store.blockAt(1)->id;

    QVERIFY(store.splitBlock(paragraphId, 4));
    const be::BlockId tailId = store.blockAt(2)->id;
    QVERIFY(store.mergeBlocks(paragraphId, tailId));

    QCOMPARE(store.toMarkdown(), original);
}

void CoreTests::splitUsesUtf16CursorOffset()
{
    be::DocumentStore store;
    store.loadMarkdown(QString::fromUtf8("a\xF0\x9F\x98\x80""b\n")); // "a<emoji>b"
    const be::BlockId id = store.blockAt(0)->id;

    // The emoji occupies two UTF-16 code units, so offset 3 sits right after it.
    QVERIFY(store.splitBlock(id, 3));
    QCOMPARE(store.blockAt(0)->markdown(), QString::fromUtf8("a\xF0\x9F\x98\x80"));
    QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("b"));
}

void CoreTests::listItemSplitContinuesMarker()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("- foo\n"));
    const be::BlockId id = store.blockAt(0)->id;

    QVERIFY(store.splitBlock(id, 5));
    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("- foo"));
    QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("- "));
    QCOMPARE(store.blockAt(1)->type, be::BlockType::BulletListItem);
}

void CoreTests::emptyListItemSplitExitsList()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("- foo\n"));
    const be::BlockId id = store.blockAt(0)->id;

    QVERIFY(store.splitBlock(id, 5));
    const be::BlockId emptyItemId = store.blockAt(1)->id;
    // Enter on the now-empty continued item exits the list.
    QVERIFY(store.splitBlock(emptyItemId, store.blockAt(1)->markdown().size()));
    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(1)->type, be::BlockType::Paragraph);
    QVERIFY(store.blockAt(1)->markdown().isEmpty());
}

void CoreTests::orderedListRenumbersOnSplit()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("1. a\n2. b\n"));
    const be::BlockId firstId = store.blockAt(0)->id;

    QVERIFY(store.splitBlock(firstId, 4));
    QCOMPARE(store.blockCount(), qsizetype(3));
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("1. a"));
    QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("2. "));
    QCOMPARE(store.blockAt(2)->markdown(), QStringLiteral("3. b"));
    QCOMPARE(store.toMarkdown(), QStringLiteral("1. a\n2. \n3. b\n"));
}

void CoreTests::headingSplitTailBecomesParagraph()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("# Title\n"));
    const be::BlockId id = store.blockAt(0)->id;

    QVERIFY(store.splitBlock(id, 7));
    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Heading);
    QCOMPARE(store.blockAt(1)->type, be::BlockType::Paragraph);
    QVERIFY(store.blockAt(1)->markdown().isEmpty());
}

void CoreTests::codeFenceEnterExitsToNewParagraph()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("```js\ncode\n```\n"));
    const be::BlockId id = store.blockAt(0)->id;
    QCOMPARE(store.blockAt(0)->type, be::BlockType::CodeFence);

    // First Enter at end of the closed fence opens a trailing blank line; the
    // block does not split yet.
    QVERIFY(store.splitBlock(id, store.blockAt(0)->markdown().size()));
    QCOMPARE(store.blockCount(), qsizetype(1));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::CodeFence);

    // Second Enter on that blank line (last line is a valid closing fence) exits
    // the fence into a new empty paragraph below it.
    QVERIFY(store.splitBlock(id, store.blockAt(0)->markdown().size()));
    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::CodeFence);
    QCOMPARE(store.blockAt(1)->type, be::BlockType::Paragraph);
    QVERIFY(store.blockAt(1)->markdown().isEmpty());
    // The fence delimiters survive the exit intact.
    QVERIFY(store.toMarkdown().startsWith(QStringLiteral("```js\ncode\n```")));
}

void CoreTests::codeFenceEnterInBodyStaysInFence()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("```js\ncode\n```\n"));
    const be::BlockId id = store.blockAt(0)->id;

    // Enter in the middle of the body inserts a literal newline and keeps a
    // single CodeFence block (no split, no exit).
    QVERIFY(store.splitBlock(id, 9)); // inside "code"
    QCOMPARE(store.blockCount(), qsizetype(1));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::CodeFence);

    // A first Enter at end (no trailing blank yet) also stays in the fence; it
    // only opens the trailing blank line that a subsequent Enter would exit on.
    be::DocumentStore store2;
    store2.loadMarkdown(QStringLiteral("```js\ncode\n```\n"));
    const be::BlockId id2 = store2.blockAt(0)->id;
    QVERIFY(store2.splitBlock(id2, store2.blockAt(0)->markdown().size()));
    QCOMPARE(store2.blockCount(), qsizetype(1));
    QCOMPARE(store2.blockAt(0)->type, be::BlockType::CodeFence);
}

void CoreTests::streamMatchesOneShotLoad()
{
    const QString markdown = QStringLiteral(
        "# Title\n\nPara\n\n```\ncode\n\n```\n\n- a\n- b\n");

    be::DocumentStore oneShot;
    oneShot.loadMarkdown(markdown);

    // The fenced block (and its ``` delimiters) must survive the parse intact.
    QCOMPARE(oneShot.toMarkdown(), markdown);

    // Stream the same content one character at a time; a blank line inside the
    // fence must not split it, and the result must match the one-shot parse.
    for (int chunkSize : {1, 3, 7}) {
        be::DocumentStore streamed;
        streamInChunks(streamed, markdown, chunkSize);

        QCOMPARE(streamed.toMarkdown(), markdown);
        QCOMPARE(streamed.toMarkdown(), oneShot.toMarkdown());
        QCOMPARE(streamed.blockCount(), oneShot.blockCount());
        for (qsizetype row = 0; row < oneShot.blockCount(); ++row) {
            QCOMPARE(streamed.blockAt(row)->type, oneShot.blockAt(row)->type);
            QCOMPARE(streamed.blockAt(row)->markdown(), oneShot.blockAt(row)->markdown());
        }
    }
}

void CoreTests::streamCommitsBlockIdentity()
{
    be::DocumentStore store;
    store.beginStreamAtEnd();

    store.streamAppend(QStringLiteral("# Title"));
    store.streamAppend(QStringLiteral("\n\nPara"));
    // The heading is now followed by a paragraph, so it has committed.
    QVERIFY(store.blockCount() >= 2);
    const be::BlockId headingId = store.blockAt(0)->id;
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Heading);

    // Appending more never disturbs the already-committed heading's identity.
    store.streamAppend(QStringLiteral(" more\n\nTail"));
    store.endStream();
    QCOMPARE(store.blockAt(0)->id, headingId);
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("# Title"));
}

void CoreTests::streamSetextReclassifies()
{
    be::DocumentStore store;
    store.beginStreamAtEnd();
    store.streamAppend(QStringLiteral("Title"));
    QCOMPARE(store.blockCount(), qsizetype(1));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Paragraph);

    // The underline turns the still-volatile paragraph into a setext heading.
    store.streamAppend(QStringLiteral("\n===\n"));
    store.endStream();
    QCOMPARE(store.blockCount(), qsizetype(1));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Heading);
}

void CoreTests::streamRoundTripThenStructuralEdit()
{
    const QString markdown = QStringLiteral("# Title\n\nParagraph\n\n- a\n- b\n");
    be::DocumentStore store;
    streamInChunks(store, markdown, 2);
    QCOMPARE(store.toMarkdown(), markdown);

    // The coordinate map / piece table were rebuilt on endStream, so a follow-up
    // structural edit still works correctly.
    const be::BlockId paragraphId = store.blockAt(1)->id;
    QVERIFY(store.splitBlock(paragraphId, 4));
    const be::BlockId tailId = store.blockAt(2)->id;
    QVERIFY(store.mergeBlocks(paragraphId, tailId));
    QCOMPARE(store.toMarkdown(), markdown);
}

void CoreTests::streamUndoRemovesAppendedBlocks()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("Existing\n"));
    const qsizetype startRow = store.blockCount();
    const be::BlockId existingId = store.blockAt(0)->id;

    streamInChunks(store, QStringLiteral("# Added\n\nMore\n"), 4);
    QVERIFY(store.blockCount() > startRow);

    QVector<be::BlockRecord> appended;
    for (qsizetype row = startRow; row < store.blockCount(); ++row) {
        appended.push_back(*store.blockAt(row));
    }

    be::StreamCommand command(startRow, {}, appended);
    command.undo(store);
    QCOMPARE(store.blockCount(), startRow);
    QCOMPARE(store.toMarkdown(), QStringLiteral("Existing\n"));
    QCOMPARE(store.blockAt(0)->id, existingId);

    command.redo(store);
    QCOMPARE(store.toMarkdown(), QStringLiteral("Existing\n\n# Added\n\nMore\n"));
}

void CoreTests::multilineParagraphRoundTrips()
{
    const QString markdown = QStringLiteral("a\nb\n\nc\n");
    be::DocumentStore store;
    store.loadMarkdown(markdown);

    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Paragraph);
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("a\nb"));
    QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("c"));
    QCOMPARE(store.toMarkdown(), markdown);
}

void CoreTests::paragraphBreakTrimsBlankLine()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("foo\n"));
    const be::BlockId id = store.blockAt(0)->id;
    // Simulate the state after a soft newline (caret on the empty second line).
    QVERIFY(store.replaceBlockMarkdown(id, QStringLiteral("foo\n")));

    QVERIFY(store.splitBlock(id, 4, nullptr, nullptr, /*trimBoundary=*/true));
    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("foo"));
    QVERIFY(store.blockAt(1)->markdown().isEmpty());
    QCOMPARE(store.toMarkdown(), QStringLiteral("foo\n\n\n"));
}

void CoreTests::paragraphBreakTrimsTrailingSpaces()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("placeholder\n"));
    const be::BlockId id = store.blockAt(0)->id;
    QVERIFY(store.replaceBlockMarkdown(id, QStringLiteral("foo  ")));

    QVERIFY(store.splitBlock(id, 5, nullptr, nullptr, /*trimBoundary=*/true));
    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("foo"));
    QVERIFY(store.blockAt(1)->markdown().isEmpty());
}

void CoreTests::paragraphBreakPreservesInternalSoftBreak()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("placeholder\n"));
    const be::BlockId id = store.blockAt(0)->id;
    QVERIFY(store.replaceBlockMarkdown(id, QStringLiteral("a\nb\n")));

    QVERIFY(store.splitBlock(id, 4, nullptr, nullptr, /*trimBoundary=*/true));
    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("a\nb"));
    QVERIFY(store.blockAt(1)->markdown().isEmpty());
}

void CoreTests::nestedListRoundTrips()
{
    const QString markdown = QStringLiteral("- a\n  - b\n  - c\n- d\n");
    be::DocumentStore store;
    store.loadMarkdown(markdown);

    QCOMPARE(store.blockCount(), qsizetype(4));
    QCOMPARE(store.blockAt(0)->indent, quint16(0));
    QCOMPARE(store.blockAt(1)->indent, quint16(2));
    QCOMPARE(store.blockAt(2)->indent, quint16(2));
    QCOMPARE(store.blockAt(3)->indent, quint16(0));
    QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("  - b"));
    QCOMPARE(store.toMarkdown(), markdown);
}

void CoreTests::indentRespectsMaxDepth()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("- a\n- b\n"));
    const be::BlockId first = store.blockAt(0)->id;
    const be::BlockId second = store.blockAt(1)->id;

    // The first item of a run has no parent to nest under.
    QVERIFY(!store.adjustListIndent(first, 1));
    QCOMPARE(store.blockAt(0)->indent, quint16(0));

    // The second item may nest one level beneath the first.
    QVERIFY(store.adjustListIndent(second, 1));
    QCOMPARE(store.blockAt(1)->indent, quint16(2));
    QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("  - b"));

    // But not two levels deeper than its predecessor.
    QVERIFY(!store.adjustListIndent(second, 1));
    QCOMPARE(store.blockAt(1)->indent, quint16(2));
}

void CoreTests::outdentFloorsAtZero()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("- a\n"));
    QVERIFY(!store.adjustListIndent(store.blockAt(0)->id, -1));
    QCOMPARE(store.blockAt(0)->indent, quint16(0));
}

void CoreTests::backspacePolicyOutdentUnlistMerge()
{
    // Nested list item -> outdent one level (and report the caret shift).
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("- a\n  - b\n"));
        qsizetype delta = 0;
        QVERIFY(store.adjustListIndent(store.blockAt(1)->id, -1, &delta));
        QCOMPARE(delta, qsizetype(-2));
        QCOMPARE(store.blockAt(1)->indent, quint16(0));
        QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("- b"));
    }

    // Top-level list item -> unlist into a paragraph in place.
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("- a\n"));
        QVERIFY(store.unlistItem(store.blockAt(0)->id));
        QCOMPARE(store.blockAt(0)->type, be::BlockType::Paragraph);
        QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("a"));
    }

    // Non-list block -> merge into the previous block.
    {
        be::DocumentStore store;
        store.loadMarkdown(QStringLiteral("foo\n\nbar\n"));
        const be::BlockId previous = store.blockAt(0)->id;
        const be::BlockId current = store.blockAt(1)->id;
        QVERIFY(store.mergeBlocks(previous, current));
        QCOMPARE(store.blockCount(), qsizetype(1));
        QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("foobar"));
    }
}

void CoreTests::nestedOrderedRenumbering()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("1. a\n2. b\n3. c\n4. d\n"));

    // Nest b and c under a; d stays at the top level.
    QVERIFY(store.adjustListIndent(store.blockAt(1)->id, 1));
    QVERIFY(store.adjustListIndent(store.blockAt(2)->id, 1));

    // Top level renumbers independently of the nested run.
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("1. a"));
    QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("  1. b"));
    QCOMPARE(store.blockAt(2)->markdown(), QStringLiteral("  2. c"));
    QCOMPARE(store.blockAt(3)->markdown(), QStringLiteral("2. d"));
}

void CoreTests::mixedListRunDepthAndSeparators()
{
    const QString markdown = QStringLiteral("- a\n  - b\n- c\n\npara\n");
    be::DocumentStore store;
    store.loadMarkdown(markdown);

    QCOMPARE(store.blockCount(), qsizetype(4));
    QCOMPARE(store.blockAt(0)->indent, quint16(0));
    QCOMPARE(store.blockAt(1)->indent, quint16(2));
    QCOMPARE(store.blockAt(2)->indent, quint16(0));
    QCOMPARE(store.blockAt(2)->type, be::BlockType::BulletListItem);
    QCOMPARE(store.blockAt(3)->type, be::BlockType::Paragraph);
    // Single newline between list items, blank line before the paragraph.
    QCOMPARE(store.toMarkdown(), markdown);
}

void CoreTests::tableClassifiedFromContent()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("placeholder\n"));
    const be::BlockId id = store.blockAt(0)->id;

    QVERIFY(store.replaceBlockMarkdown(
        id, QStringLiteral("| L | R |\n| :-- | --: |\n| a | b |")));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Table);

    // A paragraph that merely contains a stray pipe must not be mistaken for a
    // table (the second line is not a delimiter row).
    QVERIFY(store.replaceBlockMarkdown(
        id, QStringLiteral("Just a | pipe\nplain text")));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Paragraph);
}

void CoreTests::tableRoundTrips()
{
    const QString markdown =
        QStringLiteral("| Name | Age |\n| :--- | ---: |\n| Bob | 3 |\n");
    be::DocumentStore store;
    store.loadMarkdown(markdown);

    QCOMPARE(store.blockCount(), qsizetype(1));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Table);
    QCOMPARE(store.toMarkdown(), markdown);
}

void CoreTests::tableParseStructure()
{
    const be::TableData table = be::parseTable(
        QStringLiteral("| L | C | R |\n| :-- | :--: | --: |\n| a | b | c |"));

    QCOMPARE(table.columns, 3);
    QCOMPARE(table.alignments.size(), qsizetype(3));
    // md4qt alignment enum: 0=left, 1=right, 2=center.
    QCOMPARE(table.alignments[0], 0);
    QCOMPARE(table.alignments[1], 2);
    QCOMPARE(table.alignments[2], 1);

    QCOMPARE(table.header.size(), qsizetype(3));
    QCOMPARE(table.header[0].raw, QStringLiteral("L"));
    QCOMPARE(table.header[1].raw, QStringLiteral("C"));
    QCOMPARE(table.header[2].raw, QStringLiteral("R"));
    QCOMPARE(table.rows.size(), qsizetype(1));
    QCOMPARE(table.rows[0].size(), qsizetype(3));
    QCOMPARE(table.rows[0][0].raw, QStringLiteral("a"));
    QCOMPARE(table.rows[0][1].raw, QStringLiteral("b"));
    QCOMPARE(table.rows[0][2].raw, QStringLiteral("c"));
}

void CoreTests::tableParsePipesInCodeAndEscapes()
{
    // A backslash-escaped pipe is literal cell text, not a column separator. A
    // naive split-on-'|' scanner would wrongly produce three cells here; the
    // md4qt-backed split keeps two, validating GFM-correct handling.
    const be::TableData escaped = be::parseTable(
        QStringLiteral("| A | B |\n| --- | --- |\n| x \\| y | z |"));
    QCOMPARE(escaped.columns, 2);
    QCOMPARE(escaped.rows.size(), qsizetype(1));
    QCOMPARE(escaped.rows[0].size(), qsizetype(2));
    QVERIFY(escaped.rows[0][0].raw.contains(QLatin1Char('|')));
    QCOMPARE(escaped.rows[0][1].raw, QStringLiteral("z"));

    // An escaped pipe inside an inline-code span likewise stays within one cell
    // (an unescaped pipe, even inside backticks, is a GFM delimiter).
    const be::TableData code = be::parseTable(
        QStringLiteral("| Code |\n| --- |\n| `a \\| b` |"));
    QCOMPARE(code.columns, 1);
    QCOMPARE(code.rows.size(), qsizetype(1));
    QCOMPARE(code.rows[0].size(), qsizetype(1));
    QVERIFY(code.rows[0][0].raw.contains(QLatin1Char('|')));
}

void CoreTests::tableCellRawOffsets()
{
    // Each cell's [rawStart, rawEnd) must index the block markdown back to the
    // cell's trimmed raw text, even with leading padding and inline markup.
    const QString markdown =
        QStringLiteral("| Name | Tag |\n| --- | --- |\n|  **x**  | y |");
    const be::TableData table = be::parseTable(markdown);
    QCOMPARE(table.columns, 2);

    const auto verifyCell = [&](const be::TableCell &cell) {
        QCOMPARE(markdown.mid(cell.rawStart, cell.rawEnd - cell.rawStart), cell.raw);
        QVERIFY(cell.rawStart <= cell.rawEnd);
    };
    verifyCell(table.header[0]);
    verifyCell(table.header[1]);
    QCOMPARE(table.rows.size(), qsizetype(1));
    verifyCell(table.rows[0][0]);
    verifyCell(table.rows[0][1]);

    // The padded "**x**" body cell aligns rawStart to the first non-space char,
    // so the slice is exactly the trimmed inline-markup content.
    QCOMPARE(table.rows[0][0].raw, QStringLiteral("**x**"));
    QCOMPARE(markdown.at(table.rows[0][0].rawStart), QLatin1Char('*'));

    // An escaped pipe stays inside the cell; offsets still round-trip.
    const QString escaped = QStringLiteral("| A |\n| --- |\n| x \\| y |");
    const be::TableData esc = be::parseTable(escaped);
    QCOMPARE(esc.rows[0].size(), qsizetype(1));
    QCOMPARE(escaped.mid(esc.rows[0][0].rawStart, esc.rows[0][0].rawEnd - esc.rows[0][0].rawStart),
             esc.rows[0][0].raw);
    QVERIFY(esc.rows[0][0].raw.contains(QLatin1Char('|')));

    // An empty cell collapses to a zero-length range (never negative) so
    // hit-testing and span intersection degrade cleanly.
    const QString withEmpty = QStringLiteral("| A | B |\n| --- | --- |\n| | z |");
    const be::TableData emptyTable = be::parseTable(withEmpty);
    QCOMPARE(emptyTable.rows[0].size(), qsizetype(2));
    QVERIFY(emptyTable.rows[0][0].raw.isEmpty());
    QCOMPARE(emptyTable.rows[0][0].rawStart, emptyTable.rows[0][0].rawEnd);
    QVERIFY(emptyTable.rows[0][0].rawStart >= 0);
    QCOMPARE(emptyTable.rows[0][1].raw, QStringLiteral("z"));

    // An empty trailing header cell (md4qt can report degenerate positions for
    // it) also collapses to a zero-length range rather than capturing a pipe.
    const be::TableData headerEmpty =
        be::parseTable(QStringLiteral("| A |  |\n| --- | --- |\n| 1 | 2 |"));
    QCOMPARE(headerEmpty.header.size(), qsizetype(2));
    QVERIFY(headerEmpty.header[1].raw.isEmpty());
    QCOMPARE(headerEmpty.header[1].rawStart, headerEmpty.header[1].rawEnd);
}

void CoreTests::tableSurvivesEdit()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("| A | B |\n| --- | --- |\n| 1 | 2 |\n"));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Table);

    const be::BlockId id = store.blockAt(0)->id;
    QVERIFY(store.replaceBlockMarkdown(
        id, QStringLiteral("| A | B |\n| --- | --- |\n| 1 | 2 |\n| 3 | 4 |")));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Table);
}

void CoreTests::tableSplitCreatesTrailingParagraph()
{
    // A double-Enter at the end of a table (the controller's splitParagraph path,
    // i.e. splitBlock with trimBoundary) must keep the table intact and append a
    // fresh empty paragraph, mirroring paragraph escape behavior.
    const QString markdown =
        QStringLiteral("| A | B |\n| --- | --- |\n| 1 | 2 |\n");
    be::DocumentStore store;
    store.loadMarkdown(markdown);
    QCOMPARE(store.blockCount(), qsizetype(1));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Table);

    const be::BlockId id = store.blockAt(0)->id;
    const qsizetype endOffset = store.blockAt(0)->markdown().size();

    be::BlockId resultBlock = 0;
    qsizetype resultCursor = -1;
    QVERIFY(store.splitBlock(id, endOffset, &resultBlock, &resultCursor, /*trimBoundary=*/true));

    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Table);
    QCOMPARE(store.blockAt(1)->type, be::BlockType::Paragraph);
    QVERIFY(store.blockAt(1)->markdown().isEmpty());
    QCOMPARE(store.blockAt(1)->id, resultBlock);
    // The table half is unchanged and still serializes as a GFM table.
    QVERIFY(store.blockAt(0)->markdown().startsWith(QStringLiteral("| A | B |")));
}

void CoreTests::pasteHeadingDocIntoHeadingNoLeadingNewline()
{
    // Primary regression: pasting a document that itself starts with a heading
    // (no leading blank line) at the end of an existing heading must NOT merge
    // the pasted "# Title" into "# Heading"; each pasted block stays its own.
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("# Heading\n"));
    QCOMPARE(store.blockCount(), qsizetype(1));

    const be::BlockId id = store.blockAt(0)->id;
    const qsizetype end = store.blockAt(0)->markdown().size();

    be::BlockId caretBlock = 0;
    qsizetype caretOffset = -1;
    QVERIFY(store.pasteMarkdown(id, end, end,
        QStringLiteral("# Title\n\npara\n\n- item"), &caretBlock, &caretOffset));

    QCOMPARE(store.blockCount(), qsizetype(4));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Heading);
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("# Heading"));
    QCOMPARE(store.blockAt(0)->id, id); // identity preserved
    QCOMPARE(store.blockAt(1)->type, be::BlockType::Heading);
    QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("# Title"));
    QCOMPARE(store.blockAt(2)->type, be::BlockType::Paragraph);
    QCOMPARE(store.blockAt(2)->markdown(), QStringLiteral("para"));
    QCOMPARE(store.blockAt(3)->type, be::BlockType::BulletListItem);

    // Caret at the end of the last pasted block.
    QCOMPARE(caretBlock, store.blockAt(3)->id);
    QCOMPARE(caretOffset, store.blockAt(3)->markdown().size());

    QCOMPARE(store.toMarkdown(), QStringLiteral("# Heading\n\n# Title\n\npara\n\n- item\n"));
}

void CoreTests::pasteSplitsMidParagraph()
{
    // Pasting multi-block content into the middle of a paragraph keeps the
    // before/after halves as their own blocks around the inserted blocks.
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("alpha omega\n"));
    const be::BlockId id = store.blockAt(0)->id;

    be::BlockId caretBlock = 0;
    qsizetype caretOffset = -1;
    // Cursor between "alpha " and "omega" (offset 6).
    QVERIFY(store.pasteMarkdown(id, 6, 6,
        QStringLiteral("one\n\ntwo"), &caretBlock, &caretOffset));

    QCOMPARE(store.blockCount(), qsizetype(4));
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("alpha "));
    QCOMPARE(store.blockAt(1)->markdown(), QStringLiteral("one"));
    QCOMPARE(store.blockAt(2)->markdown(), QStringLiteral("two"));
    QCOMPARE(store.blockAt(3)->markdown(), QStringLiteral("omega"));
    QCOMPARE(caretBlock, store.blockAt(2)->id);
    QCOMPARE(caretOffset, qsizetype(3)); // end of "two"
}

void CoreTests::pasteSingleLineMergesInline()
{
    // A single-line paste with no block boundary merges inline (no new blocks).
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("hello world\n"));
    const be::BlockId id = store.blockAt(0)->id;

    be::BlockId caretBlock = 0;
    qsizetype caretOffset = -1;
    // Insert " there" after "hello" (offset 5).
    QVERIFY(store.pasteMarkdown(id, 5, 5, QStringLiteral(" there"), &caretBlock, &caretOffset));

    QCOMPARE(store.blockCount(), qsizetype(1));
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("hello there world"));
    QCOMPARE(caretBlock, id);
    QCOMPARE(caretOffset, qsizetype(11)); // after "hello there"
}

QTEST_MAIN(CoreTests)

#include "core_tests.moc"
