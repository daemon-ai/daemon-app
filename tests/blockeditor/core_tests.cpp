#include "core/agent_block.h"
#include "core/agent_ingest.h"
#include "core/coordinate_map.h"
#include "core/block_height_index.h"
#include "core/command_stack.h"
#include "core/document_store.h"
#include "core/image_url.h"
#include "core/inline_projector.h"
#include "core/markdown_parser.h"
#include "core/markdown_table.h"
#include "core/math_url.h"
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
    void fenceDoesNotAbsorbFollowingBlocks_data();
    void fenceDoesNotAbsorbFollowingBlocks();
    void projectionMapsMarkdown();
    void projectionOffsetEndpointsRoundTrip();
    void projectionRendersLinks();
    void linkSelectionCopiesFullSyntax();
    void linkNegativeCasesNotLinks();
    void linkInsideEmphasisIsClickable();
    void headingAnchorResolvesToRow();
    void imageBlocksClassify();
    void inlineImagesProject();
    void inlineMathProjectsAsImage();
    void displayMathInlineProjects();
    void blockMathDataDetected();
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

    // Agent transcript blocks (hybrid: typed inject/update + markdown round-trip).
    void agentBlockSerializeIsStable();
    void agentFenceParsesToTypedBlock();
    void agentBlockMarkdownRoundTrips();
    void injectTypedBlockAppendsRow();
    void updateBlockMetadataByCallIdFlipsStatus();
    void buildToolViewFlipsTitleAndDuration();
    void toolViewDerivesClarifyVariant();
    void toolViewDerivesImageGenerate();
    void toolViewFlagsAwaitingApproval();
    void clarifyAnswerRoundTrips();
    void clarifyMultiQuestionAnswerRoundTrips();
    void approvalAnswerRoundTrips();
    void clarifyAnswerPatchContract();
    void toolApprovalPatchContract();
    void ansiSpansParseSgr();
    void unifiedDiffTypesLines();
    void ingestShimReplayBuildsBlocks();
    void ingestFlushClosesTextStream();
    void agentBlocksSceneParses();
    void messageMarkerDerivesRolesAndDropsRows();
    void messageMarkerRoundTripIsStable();
    void beginMessageTagsTypedAndStreamBlocks();
    void ingestOpensAssistantMessage();
    void editUserMessageTruncatesAndRetags();
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

void CoreTests::fenceDoesNotAbsorbFollowingBlocks_data()
{
    QTest::addColumn<QString>("markdown");
    QTest::addColumn<int>("expectedFences");
    QTest::addColumn<int>("expectedListItems");
    QTest::addColumn<bool>("checkRoundTrip");

    // Regression: a fence's recovered span must stop at its own closing
    // delimiter and never swallow the following blocks (which previously rendered
    // the list inside the code card and dropped the real list rows).
    QTest::newRow("list after fence")
        << QStringLiteral("```js\ncode\n```\n\n- a\n- b\n") << 1 << 2 << true;
    QTest::newRow("list between fences")
        << QStringLiteral("```\nx\n```\n\n- a\n- b\n\n```\ny\n```\n") << 2 << 2 << true;
    // A longer (````) fence whose body contains a lone ``` line: the naive
    // nearest-marker scan would close early on the inner ```; md4qt's delimiters
    // pair the real ```` open/close, keeping the body intact and the list separate.
    QTest::newRow("longer fence with inner backticks")
        << QStringLiteral("````\ninner\n```\nstill code\n````\n\n- a\n- b\n") << 1 << 2 << true;
    // Indented (4-space) code is not fenced and must not trigger marker scanning;
    // the following list stays its own blocks. (Indented-code indentation handling
    // is pre-existing and out of scope, so the round-trip is not asserted here.)
    QTest::newRow("indented code then list")
        << QStringLiteral("    code line\n\n- a\n- b\n") << 1 << 2 << false;
    // An unterminated fence keeps its opening delimiter and runs to end-of-text.
    QTest::newRow("unterminated fence")
        << QStringLiteral("```js\ncode\nmore\n") << 1 << 0 << true;
}

void CoreTests::fenceDoesNotAbsorbFollowingBlocks()
{
    QFETCH(QString, markdown);
    QFETCH(int, expectedFences);
    QFETCH(int, expectedListItems);
    QFETCH(bool, checkRoundTrip);

    be::DocumentStore store;
    store.loadMarkdown(markdown);

    if (checkRoundTrip) {
        QCOMPARE(store.toMarkdown(), markdown);
    }

    int fences = 0;
    int listItems = 0;
    for (qsizetype row = 0; row < store.blockCount(); ++row) {
        const be::BlockType type = store.blockAt(row)->type;
        if (type == be::BlockType::CodeFence) {
            ++fences;
        } else if (type == be::BlockType::BulletListItem
                   || type == be::BlockType::OrderedListItem
                   || type == be::BlockType::TaskListItem) {
            ++listItems;
        }
    }
    QCOMPARE(fences, expectedFences);
    QCOMPARE(listItems, expectedListItems);

    // The list rows must keep their own bullet markdown rather than being folded
    // into a code block's stored text.
    for (qsizetype row = 0; row < store.blockCount(); ++row) {
        const be::BlockRecord *block = store.blockAt(row);
        if (block->type == be::BlockType::CodeFence) {
            QVERIFY(!block->markdown().contains(QStringLiteral("\n- a")));
        }
    }
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

void CoreTests::inlineMathProjectsAsImage()
{
    be::InlineProjector projector;

    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("a $x^2$ b\n"));
    QVERIFY(store.blockCount() >= 1);
    const be::BlockRecord *block = store.blockAt(0);
    QCOMPARE(block->type, be::BlockType::Paragraph);

    const be::BlockProjection p = projector.project(*block);

    // The formula collapses to a single object-replacement placeholder, keeping
    // the visual<->raw offset maps aligned (visual text is "a \uFFFC b").
    int placeholders = 0;
    for (const QChar c : p.visualText) {
        if (c == QChar(0xFFFC)) {
            ++placeholders;
        }
    }
    QCOMPARE(placeholders, 1);

    // Exactly one math <img> and exactly one Math span.
    QCOMPARE(p.displayMarkup.count(QStringLiteral("<img src=\"image://math/")), 1);
    QVERIFY(!p.displayMarkup.contains(QStringLiteral("<img src=\"image://imgcache")));
    int mathSpans = 0;
    for (const be::InlineSpan &span : p.spans) {
        if (span.kind == be::SpanKind::Math) {
            ++mathSpans;
            QCOMPARE(span.mathLatex, QStringLiteral("x^2"));
            QVERIFY(!span.mathDisplay);
        }
    }
    QCOMPARE(mathSpans, 1);

    // The placeholder's raw range covers the whole $...$, so selecting the block
    // copies back the full Markdown (offset-map round-trip).
    be::SelectionControllerCore selection;
    selection.setAnchor({block->id, 0, p.visualToRaw(0), 0});
    selection.setHead({block->id, 0, p.visualToRaw(p.visualText.size()), p.visualText.size()});
    QCOMPARE(selection.copyMarkdown(store.blocks()), QStringLiteral("a $x^2$ b"));
}

void CoreTests::displayMathInlineProjects()
{
    be::InlineProjector projector;

    be::BlockRecord block;
    block.id = 1;
    block.type = be::BlockType::Paragraph;
    block.markdownUtf8 = QByteArrayLiteral("see $$\\int$$ here");
    const be::BlockProjection p = projector.project(block);

    QVERIFY(p.displayMarkup.contains(QStringLiteral("<img src=\"image://math/")));
    int mathSpans = 0;
    for (const be::InlineSpan &span : p.spans) {
        if (span.kind == be::SpanKind::Math) {
            ++mathSpans;
            QCOMPARE(span.mathLatex, QStringLiteral("\\int"));
            QVERIFY(span.mathDisplay);
        }
    }
    QCOMPARE(mathSpans, 1);
}

void CoreTests::blockMathDataDetected()
{
    // A whole-block $$...$$ span yields the stripped LaTeX (single- and
    // multi-line forms).
    QCOMPARE(be::blockMathSource(QStringLiteral("$$x^2 + y^2$$")),
             QStringLiteral("x^2 + y^2"));
    QCOMPARE(be::blockMathSource(QStringLiteral("$$\n\\frac{a}{b}\n$$")),
             QStringLiteral("\\frac{a}{b}"));

    // A ```math fence yields its body with the fence lines stripped.
    QCOMPARE(be::blockMathSource(QStringLiteral("```math\n\\frac{a}{b}\n```")),
             QStringLiteral("\\frac{a}{b}"));

    // A normal paragraph and inline-only math are not block math.
    QVERIFY(be::blockMathSource(QStringLiteral("just a paragraph")).isEmpty());
    QVERIFY(be::blockMathSource(QStringLiteral("inline $x$ math")).isEmpty());
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

void CoreTests::agentBlockSerializeIsStable()
{
    QVariantMap meta;
    meta.insert(QStringLiteral("callId"), QStringLiteral("c1"));
    meta.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    meta.insert(QStringLiteral("status"), QStringLiteral("ok"));
    meta.insert(QStringLiteral("durationMs"), 1200);

    const QByteArray md = be::serializeAgentBlock(be::BlockType::ToolCall, meta);
    QVERIFY(md.startsWith("```tool\n"));
    QVERIFY(md.endsWith("\n```"));

    // Parsing the body back, then re-serializing, must be byte-stable (the JSON
    // object is key-sorted), so this form is a safe document save/export format.
    const QVariantMap parsed = be::parseAgentBlockMetadata(be::fencedBodyOf(QString::fromUtf8(md)));
    const QByteArray md2 = be::serializeAgentBlock(be::BlockType::ToolCall, parsed);
    QCOMPARE(md2, md);

    // The transient fenceLanguage key the parser stamps on a fence never leaks.
    QVariantMap withFence = meta;
    withFence.insert(QStringLiteral("fenceLanguage"), QStringLiteral("tool"));
    QCOMPARE(be::serializeAgentBlock(be::BlockType::ToolCall, withFence), md);
}

void CoreTests::agentFenceParsesToTypedBlock()
{
    const QString markdown = QStringLiteral(
        "```reasoning\n{\"body\":\"Think first.\",\"status\":\"complete\"}\n```\n");
    be::DocumentStore store;
    store.loadMarkdown(markdown);

    const be::BlockRecord *block = store.blockAt(0);
    QVERIFY(block != nullptr);
    QCOMPARE(block->type, be::BlockType::Reasoning);
    QCOMPARE(block->metadata.value(QStringLiteral("status")).toString(), QStringLiteral("complete"));
    QCOMPARE(block->metadata.value(QStringLiteral("body")).toString(), QStringLiteral("Think first."));
}

void CoreTests::agentBlockMarkdownRoundTrips()
{
    QVariantMap meta;
    meta.insert(QStringLiteral("callId"), QStringLiteral("c7"));
    meta.insert(QStringLiteral("name"), QStringLiteral("search"));
    meta.insert(QStringLiteral("status"), QStringLiteral("ok"));
    meta.insert(QStringLiteral("detailKind"), QStringLiteral("ansi-stream"));
    const QString md = QString::fromUtf8(be::serializeAgentBlock(be::BlockType::ToolCall, meta));

    be::DocumentStore store;
    store.loadMarkdown(md + QLatin1Char('\n'));

    const be::BlockRecord *block = store.blockAt(0);
    QVERIFY(block != nullptr);
    QCOMPARE(block->type, be::BlockType::ToolCall);
    // The block's stored markdown round-trips back to the canonical fenced form.
    QCOMPARE(block->markdown(), md);
    QCOMPARE(store.toMarkdown(), md + QLatin1Char('\n'));
    // And the document survives a structural reload via toMarkdown.
    be::DocumentStore reloaded;
    reloaded.loadMarkdown(store.toMarkdown());
    QCOMPARE(reloaded.blockAt(0)->type, be::BlockType::ToolCall);
    QCOMPARE(reloaded.blockAt(0)->metadata.value(QStringLiteral("callId")).toString(), QStringLiteral("c7"));
}

void CoreTests::injectTypedBlockAppendsRow()
{
    be::DocumentStore store;
    store.loadMarkdown(QStringLiteral("Intro\n"));
    const qsizetype before = store.blockCount();

    QVariantMap meta;
    meta.insert(QStringLiteral("callId"), QStringLiteral("c1"));
    meta.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    meta.insert(QStringLiteral("status"), QStringLiteral("running"));
    be::BlockChangeSet cs;
    const be::BlockId id = store.appendTypedBlock(be::BlockType::ToolCall, meta, &cs);

    QVERIFY(id != 0);
    QCOMPARE(store.blockCount(), before + 1);
    QCOMPARE(cs.insertedCount, qsizetype(1));
    QCOMPARE(cs.structuralRow, before);
    QCOMPARE(store.blockAt(before)->type, be::BlockType::ToolCall);
    QCOMPARE(store.blockIdForMetadata(QStringLiteral("callId"), QStringLiteral("c1")), id);
}

void CoreTests::updateBlockMetadataByCallIdFlipsStatus()
{
    be::DocumentStore store;
    QVariantMap meta;
    meta.insert(QStringLiteral("callId"), QStringLiteral("abc"));
    meta.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    meta.insert(QStringLiteral("status"), QStringLiteral("running"));
    const be::BlockId id = store.appendTypedBlock(be::BlockType::ToolCall, meta, nullptr);

    const be::BlockId found = store.blockIdForMetadata(QStringLiteral("callId"), QStringLiteral("abc"));
    QCOMPARE(found, id);

    QVariantMap patch;
    patch.insert(QStringLiteral("status"), QStringLiteral("ok"));
    patch.insert(QStringLiteral("durationMs"), 850);
    const be::BlockChangeSet cs = store.updateBlockMetadata(found, patch);

    const qsizetype row = store.rowForBlock(id);
    QCOMPARE(cs.changedFirst, row);
    QCOMPARE(cs.changedLast, row);
    QCOMPARE(store.blockAt(row)->metadata.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
    QCOMPARE(store.blockAt(row)->metadata.value(QStringLiteral("durationMs")).toInt(), 850);
    // The patch also re-serialized the canonical markdown so a later save persists it.
    QVERIFY(store.blockAt(row)->markdown().contains(QStringLiteral("\"status\":\"ok\"")));
}

void CoreTests::buildToolViewFlipsTitleAndDuration()
{
    QVariantMap running;
    running.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    running.insert(QStringLiteral("status"), QStringLiteral("running"));
    const QVariantMap rv = be::buildToolView(running);
    QCOMPARE(rv.value(QStringLiteral("status")).toString(), QStringLiteral("running"));
    QCOMPARE(rv.value(QStringLiteral("title")).toString(), QStringLiteral("Running terminal"));

    QVariantMap done;
    done.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    done.insert(QStringLiteral("status"), QStringLiteral("finished")); // normalizes to ok
    done.insert(QStringLiteral("durationMs"), 1500);
    const QVariantMap dv = be::buildToolView(done);
    QCOMPARE(dv.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
    QCOMPARE(dv.value(QStringLiteral("title")).toString(), QStringLiteral("terminal"));
    QCOMPARE(dv.value(QStringLiteral("durationLabel")).toString(), QStringLiteral("1.5s"));
}

void CoreTests::toolViewDerivesClarifyVariant()
{
    QVariantMap clarify;
    clarify.insert(QStringLiteral("name"), QStringLiteral("clarify"));
    clarify.insert(QStringLiteral("question"), QStringLiteral("Which DB?"));
    const QVariantMap v = be::buildToolView(clarify);
    QCOMPARE(v.value(QStringLiteral("variant")).toString(), QStringLiteral("clarify"));
    // An omitted tone defaults from the name.
    QCOMPARE(v.value(QStringLiteral("tone")).toString(), QStringLiteral("agent"));
    QCOMPARE(v.value(QStringLiteral("title")).toString(), QStringLiteral("Needs your input"));

    clarify.insert(QStringLiteral("answered"), true);
    const QVariantMap answered = be::buildToolView(clarify);
    QCOMPARE(answered.value(QStringLiteral("title")).toString(), QStringLiteral("Answered"));
}

void CoreTests::toolViewDerivesImageGenerate()
{
    QVariantMap gen;
    gen.insert(QStringLiteral("name"), QStringLiteral("image_generate"));
    gen.insert(QStringLiteral("status"), QStringLiteral("ok"));
    gen.insert(QStringLiteral("imageUrl"), QStringLiteral("https://example/x.png"));
    const QVariantMap v = be::buildToolView(gen);
    QCOMPARE(v.value(QStringLiteral("variant")).toString(), QStringLiteral("image-generate"));
    QCOMPARE(v.value(QStringLiteral("tone")).toString(), QStringLiteral("image"));
    // With a resolved image and no explicit detailKind, route to the frame.
    QCOMPARE(v.value(QStringLiteral("detailKind")).toString(), QStringLiteral("generated-image"));

    // While pending (no image yet) there is no detail kind to render.
    QVariantMap pending;
    pending.insert(QStringLiteral("name"), QStringLiteral("image_generate"));
    pending.insert(QStringLiteral("status"), QStringLiteral("running"));
    QCOMPARE(be::buildToolView(pending).value(QStringLiteral("detailKind")).toString(), QString());
}

void CoreTests::toolViewFlagsAwaitingApproval()
{
    QVariantMap meta;
    meta.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    meta.insert(QStringLiteral("status"), QStringLiteral("running"));
    meta.insert(QStringLiteral("needsApproval"), true);
    QVERIFY(be::buildToolView(meta).value(QStringLiteral("awaitingApproval")).toBool());

    // Once decided, the gate clears even while the tool keeps running.
    meta.insert(QStringLiteral("approval"), QStringLiteral("approved"));
    QVERIFY(!be::buildToolView(meta).value(QStringLiteral("awaitingApproval")).toBool());

    // A non-approval tool never raises the bar.
    QVariantMap plain;
    plain.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    plain.insert(QStringLiteral("status"), QStringLiteral("running"));
    QVERIFY(!be::buildToolView(plain).value(QStringLiteral("awaitingApproval")).toBool());
}

void CoreTests::clarifyAnswerRoundTrips()
{
    be::DocumentStore store;
    QVariantMap meta;
    meta.insert(QStringLiteral("callId"), QStringLiteral("q1"));
    meta.insert(QStringLiteral("name"), QStringLiteral("clarify"));
    meta.insert(QStringLiteral("question"), QStringLiteral("Which DB?"));
    const be::BlockId id = store.appendTypedBlock(be::BlockType::ToolCall, meta, nullptr);

    // The answer the controller would apply as a local echo.
    QVariantMap patch;
    patch.insert(QStringLiteral("answered"), true);
    patch.insert(QStringLiteral("answer"), QStringLiteral("PostgreSQL"));
    store.updateBlockMetadata(id, patch);

    const qsizetype row = store.rowForBlock(id);
    QVERIFY(store.blockAt(row)->metadata.value(QStringLiteral("answered")).toBool());
    QCOMPARE(store.blockAt(row)->metadata.value(QStringLiteral("answer")).toString(), QStringLiteral("PostgreSQL"));
    // The answer persists in the canonical fenced markdown and re-parses stably.
    const QString md = store.toMarkdown();
    QVERIFY(md.contains(QStringLiteral("\"answer\":\"PostgreSQL\"")));
    be::DocumentStore reloaded;
    reloaded.loadMarkdown(md);
    QCOMPARE(reloaded.toMarkdown(), md);
}

void CoreTests::clarifyMultiQuestionAnswerRoundTrips()
{
    be::DocumentStore store;
    // A clarify block carrying several questions, one of them multi-select.
    QVariantMap qDb;
    qDb.insert(QStringLiteral("id"), QStringLiteral("db"));
    qDb.insert(QStringLiteral("prompt"), QStringLiteral("Which DB?"));
    qDb.insert(QStringLiteral("choices"), QVariantList{ QStringLiteral("PostgreSQL"), QStringLiteral("SQLite") });
    QVariantMap qScope;
    qScope.insert(QStringLiteral("id"), QStringLiteral("scope"));
    qScope.insert(QStringLiteral("prompt"), QStringLiteral("What to migrate?"));
    qScope.insert(QStringLiteral("choices"), QVariantList{ QStringLiteral("Schema"), QStringLiteral("Data"), QStringLiteral("Indexes") });
    qScope.insert(QStringLiteral("multiSelect"), true);

    QVariantMap meta;
    meta.insert(QStringLiteral("callId"), QStringLiteral("q1"));
    meta.insert(QStringLiteral("name"), QStringLiteral("clarify"));
    meta.insert(QStringLiteral("questions"), QVariantList{ qDb, qScope });
    const be::BlockId id = store.appendTypedBlock(be::BlockType::ToolCall, meta, nullptr);

    // The structured answers the controller would apply: a scalar for the
    // single-select question and a list for the multi-select one.
    QVariantMap answers;
    answers.insert(QStringLiteral("db"), QStringLiteral("PostgreSQL"));
    answers.insert(QStringLiteral("scope"), QVariantList{ QStringLiteral("Schema"), QStringLiteral("Indexes") });
    QVariantMap patch;
    patch.insert(QStringLiteral("answered"), true);
    patch.insert(QStringLiteral("answers"), answers);
    patch.insert(QStringLiteral("answer"), QStringLiteral("PostgreSQL; Schema, Indexes"));
    store.updateBlockMetadata(id, patch);

    const qsizetype row = store.rowForBlock(id);
    const QVariantMap stored = store.blockAt(row)->metadata;
    QVERIFY(stored.value(QStringLiteral("answered")).toBool());
    const QVariantMap storedAnswers = stored.value(QStringLiteral("answers")).toMap();
    QCOMPARE(storedAnswers.value(QStringLiteral("db")).toString(), QStringLiteral("PostgreSQL"));
    QCOMPARE(storedAnswers.value(QStringLiteral("scope")).toStringList(),
             QStringList({ QStringLiteral("Schema"), QStringLiteral("Indexes") }));

    // The nested questions + structured answers survive a markdown round-trip.
    const QString md = store.toMarkdown();
    QVERIFY(md.contains(QStringLiteral("\"answers\":")));
    QVERIFY(md.contains(QStringLiteral("\"questions\":")));
    be::DocumentStore reloaded;
    reloaded.loadMarkdown(md);
    QCOMPARE(reloaded.toMarkdown(), md);
}

void CoreTests::approvalAnswerRoundTrips()
{
    be::DocumentStore store;
    QVariantMap meta;
    meta.insert(QStringLiteral("callId"), QStringLiteral("c8"));
    meta.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    meta.insert(QStringLiteral("status"), QStringLiteral("running"));
    meta.insert(QStringLiteral("needsApproval"), true);
    const be::BlockId id = store.appendTypedBlock(be::BlockType::ToolCall, meta, nullptr);
    QVERIFY(be::buildToolView(store.blockAt(store.rowForBlock(id))->metadata)
                .value(QStringLiteral("awaitingApproval")).toBool());

    // A denial (what answerToolApproval applies) clears the gate and errors out.
    QVariantMap patch;
    patch.insert(QStringLiteral("approval"), QStringLiteral("denied"));
    patch.insert(QStringLiteral("needsApproval"), false);
    patch.insert(QStringLiteral("status"), QStringLiteral("error"));
    store.updateBlockMetadata(id, patch);

    const QVariantMap view = be::buildToolView(store.blockAt(store.rowForBlock(id))->metadata);
    QVERIFY(!view.value(QStringLiteral("awaitingApproval")).toBool());
    QCOMPARE(view.value(QStringLiteral("status")).toString(), QStringLiteral("error"));
    QVERIFY(store.toMarkdown().contains(QStringLiteral("\"approval\":\"denied\"")));
}

void CoreTests::clarifyAnswerPatchContract()
{
    // The shared be::clarifyAnswerPatch must produce exactly what
    // EditorController::answerClarify used to build inline, so the GUI and the TUI
    // answer identically. A scalar answer and a list answer, with the flat human
    // `answer` summary joining each (lists comma-joined) with "; ".
    QVariantMap answers;
    answers.insert(QStringLiteral("db"), QStringLiteral("PostgreSQL"));
    answers.insert(QStringLiteral("scope"),
                   QVariantList { QStringLiteral("Schema"), QStringLiteral("Indexes") });

    const QVariantMap patch = be::clarifyAnswerPatch(answers);
    QCOMPARE(patch.value(QStringLiteral("answered")).toBool(), true);
    QCOMPARE(patch.value(QStringLiteral("answers")).toMap(), answers);
    // QVariantMap iterates keys sorted, so "db" precedes "scope".
    QCOMPARE(patch.value(QStringLiteral("answer")).toString(),
             QStringLiteral("PostgreSQL; Schema, Indexes"));
}

void CoreTests::toolApprovalPatchContract()
{
    // Approval clears the gate and leaves the tool running for the host to finish.
    const QVariantMap approved = be::toolApprovalPatch(QStringLiteral("approved"));
    QCOMPARE(approved.value(QStringLiteral("approval")).toString(), QStringLiteral("approved"));
    QCOMPARE(approved.value(QStringLiteral("needsApproval")).toBool(), false);
    QVERIFY(!approved.contains(QStringLiteral("status")));

    // Denial additionally flips the tool to an error.
    const QVariantMap denied = be::toolApprovalPatch(QStringLiteral("denied"));
    QCOMPARE(denied.value(QStringLiteral("approval")).toString(), QStringLiteral("denied"));
    QCOMPARE(denied.value(QStringLiteral("needsApproval")).toBool(), false);
    QCOMPARE(denied.value(QStringLiteral("status")).toString(), QStringLiteral("error"));
}

void CoreTests::ansiSpansParseSgr()
{
    // "plain \x1b[31mred\x1b[1m bold\x1b[0m done"
    const QString text = QStringLiteral("plain ")
        + QChar(0x1B) + QStringLiteral("[31mred")
        + QChar(0x1B) + QStringLiteral("[1m bold")
        + QChar(0x1B) + QStringLiteral("[0m done");
    const QVariantList spans = be::ansiToSpans(text);
    QCOMPARE(spans.size(), 4);

    QCOMPARE(spans.at(0).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("plain "));
    QCOMPARE(spans.at(0).toMap().value(QStringLiteral("fg")).toInt(), -1);

    QCOMPARE(spans.at(1).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("red"));
    QCOMPARE(spans.at(1).toMap().value(QStringLiteral("fg")).toInt(), 1); // red

    const QVariantMap bold = spans.at(2).toMap();
    QCOMPARE(bold.value(QStringLiteral("text")).toString(), QStringLiteral(" bold"));
    QCOMPARE(bold.value(QStringLiteral("fg")).toInt(), 1);
    QVERIFY(bold.value(QStringLiteral("bold")).toBool());

    const QVariantMap reset = spans.at(3).toMap();
    QCOMPARE(reset.value(QStringLiteral("text")).toString(), QStringLiteral(" done"));
    QCOMPARE(reset.value(QStringLiteral("fg")).toInt(), -1);
    QVERIFY(!reset.value(QStringLiteral("bold")).toBool());
}

void CoreTests::unifiedDiffTypesLines()
{
    const QString diff = QStringLiteral(
        "--- a/file\n+++ b/file\n@@ -1,2 +1,2 @@\n context\n-old line\n+new line\n");
    const QVariantList lines = be::parseUnifiedDiff(diff);
    QVERIFY(lines.size() >= 6);
    QCOMPARE(lines.at(0).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("meta"));
    QCOMPARE(lines.at(1).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("meta"));
    QCOMPARE(lines.at(2).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("hunk"));
    QCOMPARE(lines.at(3).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("context"));
    QCOMPARE(lines.at(4).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("del"));
    QCOMPARE(lines.at(5).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("add"));
}

void CoreTests::ingestShimReplayBuildsBlocks()
{
    be::DocumentStore store;
    be::TranscriptIngest ingest(&store);

    QVariantList feed;
    auto event = [](const char *type) {
        QVariantMap e;
        e.insert(QStringLiteral("type"), QString::fromLatin1(type));
        return e;
    };

    // A recorded turn: think, run a tool, then finish it by callId.
    QVariantMap r1 = event("reasoningDelta");
    r1.insert(QStringLiteral("text"), QStringLiteral("Let me check."));
    feed.push_back(r1);

    QVariantMap rDone = event("reasoningDone");
    rDone.insert(QStringLiteral("durationMs"), 300);
    feed.push_back(rDone);

    QVariantMap started = event("toolStarted");
    started.insert(QStringLiteral("callId"), QStringLiteral("t1"));
    started.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    started.insert(QStringLiteral("tone"), QStringLiteral("terminal"));
    started.insert(QStringLiteral("detailKind"), QStringLiteral("ansi-stream"));
    feed.push_back(started);

    QVariantMap finished = event("toolFinished");
    finished.insert(QStringLiteral("callId"), QStringLiteral("t1"));
    finished.insert(QStringLiteral("status"), QStringLiteral("ok"));
    finished.insert(QStringLiteral("durationMs"), 1200);
    finished.insert(QStringLiteral("stdout"), QStringLiteral("done\n"));
    feed.push_back(finished);

    ingest.ingestAll(feed);
    ingest.finish();

    QCOMPARE(store.blockCount(), qsizetype(2));
    QCOMPARE(store.blockAt(0)->type, be::BlockType::Reasoning);
    QCOMPARE(store.blockAt(0)->metadata.value(QStringLiteral("status")).toString(), QStringLiteral("complete"));
    QCOMPARE(store.blockAt(0)->metadata.value(QStringLiteral("body")).toString(), QStringLiteral("Let me check."));

    QCOMPARE(store.blockAt(1)->type, be::BlockType::ToolCall);
    QCOMPARE(store.blockAt(1)->metadata.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
    QCOMPARE(store.blockAt(1)->metadata.value(QStringLiteral("durationMs")).toInt(), 1200);
    QCOMPARE(store.blockAt(1)->metadata.value(QStringLiteral("stdout")).toString(), QStringLiteral("done\n"));
}

void CoreTests::ingestFlushClosesTextStream()
{
    be::DocumentStore store;
    be::TranscriptIngest ingest(&store);

    // A text event opens a streaming tail; without an explicit close the stream
    // stays open (volatile). A flush event must settle it through finish().
    QVariantMap textEvent;
    textEvent.insert(QStringLiteral("type"), QStringLiteral("text"));
    textEvent.insert(QStringLiteral("text"), QStringLiteral("Hello from the agent.\n"));
    ingest.ingest(textEvent);

    QVariantMap flush;
    flush.insert(QStringLiteral("type"), QStringLiteral("flush"));
    ingest.ingest(flush);

    // The streamed paragraph is committed, and the document is stable across a
    // reload (no dangling volatile tail to reconcile).
    QVERIFY(store.blockCount() >= 1);
    const QString md = store.toMarkdown();
    QVERIFY(md.contains(QStringLiteral("Hello from the agent.")));
    be::DocumentStore reloaded;
    reloaded.loadMarkdown(md);
    QCOMPARE(reloaded.toMarkdown(), md);

    // A second flush with nothing open is a harmless no-op.
    ingest.ingest(flush);
    QCOMPARE(store.toMarkdown(), md);
}

void CoreTests::agentBlocksSceneParses()
{
    // A miniature of the seeded "Agent blocks demo" transcript: a heading, each
    // typed block in its canonical fenced form, interleaved with prose. Asserts
    // the document parses into the right BlockTypes with hydrated metadata, and
    // that the detail payloads feed the sub-renderer parsers - the parse-level
    // proof behind the visual demo conversation.
    const QString scene = QStringLiteral(
        "# Agent transcript blocks\n\n"
        "Intro paragraph.\n\n"
        "```reasoning\n{\"status\":\"complete\",\"body\":\"Think then act.\"}\n```\n\n"
        "```tool\n{\"callId\":\"c1\",\"name\":\"terminal\",\"status\":\"ok\",\"detailKind\":\"ansi-stream\","
        "\"stdout\":\"\\u001b[32mPASS\\u001b[0m\\n\"}\n```\n\n"
        "```tool\n{\"callId\":\"c3\",\"name\":\"apply_patch\",\"status\":\"ok\",\"detailKind\":\"diff\","
        "\"diff\":\"--- a\\n+++ b\\n@@ -1 +1 @@\\n-x\\n+y\\n\"}\n```\n\n"
        "```content\n{\"kind\":\"ansi-stream\",\"body\":\"tailing\\n\"}\n```\n\n"
        "Outro paragraph.\n");

    be::DocumentStore store;
    store.loadMarkdown(scene);

    // Collect the typed blocks by walking the document.
    const be::BlockRecord *reasoning = nullptr;
    const be::BlockRecord *ansiTool = nullptr;
    const be::BlockRecord *diffTool = nullptr;
    const be::BlockRecord *content = nullptr;
    for (qsizetype row = 0; row < store.blockCount(); ++row) {
        const be::BlockRecord *b = store.blockAt(row);
        if (b->type == be::BlockType::Reasoning) {
            reasoning = b;
        } else if (b->type == be::BlockType::ToolCall) {
            if (b->metadata.value(QStringLiteral("detailKind")).toString() == QStringLiteral("diff")) {
                diffTool = b;
            } else {
                ansiTool = b;
            }
        } else if (b->type == be::BlockType::Content) {
            content = b;
        }
    }

    QVERIFY(reasoning != nullptr);
    QVERIFY(ansiTool != nullptr);
    QVERIFY(diffTool != nullptr);
    QVERIFY(content != nullptr);

    QCOMPARE(reasoning->metadata.value(QStringLiteral("status")).toString(), QStringLiteral("complete"));

    // The ansi stdout decodes into colored spans (the AnsiText input).
    const QVariantList spans = be::ansiToSpans(ansiTool->metadata.value(QStringLiteral("stdout")).toString());
    QVERIFY(spans.size() >= 2);
    QCOMPARE(spans.at(0).toMap().value(QStringLiteral("fg")).toInt(), 2); // green PASS

    // The diff decodes into typed lines (the DiffBlock input).
    const QVariantList diffLines = be::parseUnifiedDiff(diffTool->metadata.value(QStringLiteral("diff")).toString());
    QVERIFY(diffLines.size() >= 5);

    // The whole document round-trips back to the same markdown.
    QCOMPARE(store.toMarkdown(), scene);
}

void CoreTests::messageMarkerDerivesRolesAndDropsRows()
{
    // A document with msg boundary markers: each marker tags the blocks that
    // follow it and is itself consumed (never becomes a row).
    const QString md = QStringLiteral(
        "```msg\n{\"id\":\"u1\",\"role\":\"user\"}\n```\n\n"
        "Migrate the database, please.\n\n"
        "```msg\n{\"id\":\"m1\",\"role\":\"assistant\"}\n```\n\n"
        "Working on it.\n\nDone — all tables migrated.\n\n"
        "```msg\n{\"id\":\"s1\",\"role\":\"system\"}\n```\n\n"
        "steer:Use PostgreSQL syntax.\n");

    be::DocumentStore store;
    store.loadMarkdown(md);

    // Four content blocks, zero marker rows.
    QCOMPARE(store.blockCount(), qsizetype(4));

    QCOMPARE(store.blockAt(0)->role, be::MessageRole::User);
    QCOMPARE(store.blockAt(0)->messageId, QStringLiteral("u1"));

    QCOMPARE(store.blockAt(1)->role, be::MessageRole::Assistant);
    QCOMPARE(store.blockAt(1)->messageId, QStringLiteral("m1"));
    QCOMPARE(store.blockAt(2)->role, be::MessageRole::Assistant);
    QCOMPARE(store.blockAt(2)->messageId, QStringLiteral("m1"));

    QCOMPARE(store.blockAt(3)->role, be::MessageRole::System);
    QCOMPARE(store.blockAt(3)->messageId, QStringLiteral("s1"));
}

void CoreTests::messageMarkerRoundTripIsStable()
{
    const QString md = QStringLiteral(
        "```msg\n{\"id\":\"u1\",\"role\":\"user\"}\n```\n\n"
        "Hello there.\n\n"
        "```msg\n{\"id\":\"m1\",\"role\":\"assistant\"}\n```\n\n"
        "Hi! How can I help?\n");

    be::DocumentStore store;
    store.loadMarkdown(md);

    // serialize -> parse -> serialize is a fixed point: the re-emitted markers
    // land exactly at the role/messageId boundaries.
    const QString once = store.toMarkdown();
    be::DocumentStore reloaded;
    reloaded.loadMarkdown(once);
    QCOMPARE(reloaded.toMarkdown(), once);

    // The serialized form carries a marker for each message boundary.
    QVERIFY(once.contains(QStringLiteral("\"role\":\"user\"")));
    QVERIFY(once.contains(QStringLiteral("\"role\":\"assistant\"")));
}

void CoreTests::beginMessageTagsTypedAndStreamBlocks()
{
    be::DocumentStore store;

    // A user message via the runtime append path, then an assistant message that
    // mixes a streamed paragraph and a typed tool block.
    const QString userId = store.appendMessageBlocks(be::MessageRole::User, QStringLiteral("Run the build."));
    QCOMPARE(store.blockAt(0)->role, be::MessageRole::User);
    QCOMPARE(store.blockAt(0)->messageId, userId);

    const QString assistantId = store.beginMessage(be::MessageRole::Assistant);
    QVERIFY(assistantId != userId);

    store.beginStreamAtEnd();
    store.streamAppend(QStringLiteral("Building now.\n"));
    store.endStream();

    QVariantMap toolMeta;
    toolMeta.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    toolMeta.insert(QStringLiteral("status"), QStringLiteral("ok"));
    store.appendTypedBlock(be::BlockType::ToolCall, toolMeta);

    // Every assistant-turn block carries the assistant message id.
    for (qsizetype row = 1; row < store.blockCount(); ++row) {
        QCOMPARE(store.blockAt(row)->role, be::MessageRole::Assistant);
        QCOMPARE(store.blockAt(row)->messageId, assistantId);
    }

    // The serialized document round-trips with the derived roles intact.
    const QString md = store.toMarkdown();
    be::DocumentStore reloaded;
    reloaded.loadMarkdown(md);
    QCOMPARE(reloaded.toMarkdown(), md);
    QCOMPARE(reloaded.blockAt(0)->role, be::MessageRole::User);
    QCOMPARE(reloaded.blockAt(reloaded.blockCount() - 1)->role, be::MessageRole::Assistant);
}

void CoreTests::ingestOpensAssistantMessage()
{
    be::DocumentStore store;
    be::TranscriptIngest ingest(&store);

    // The first content event of a turn opens an assistant message, so streamed
    // text and typed blocks are tagged without an explicit beginMessage.
    QVariantMap textEvent;
    textEvent.insert(QStringLiteral("type"), QStringLiteral("text"));
    textEvent.insert(QStringLiteral("text"), QStringLiteral("Thinking out loud.\n"));
    ingest.ingest(textEvent);

    QVariantMap started;
    started.insert(QStringLiteral("type"), QStringLiteral("toolStarted"));
    started.insert(QStringLiteral("callId"), QStringLiteral("t1"));
    started.insert(QStringLiteral("name"), QStringLiteral("terminal"));
    ingest.ingest(started);
    ingest.finish();

    QVERIFY(store.blockCount() >= 2);
    const QString firstId = store.blockAt(0)->messageId;
    QVERIFY(!firstId.isEmpty());
    for (qsizetype row = 0; row < store.blockCount(); ++row) {
        QCOMPARE(store.blockAt(row)->role, be::MessageRole::Assistant);
        QCOMPARE(store.blockAt(row)->messageId, firstId);
    }

    // A subsequent turn opens a fresh assistant message id.
    ingest.ingest(textEvent);
    ingest.finish();
    QVERIFY(store.blockAt(store.blockCount() - 1)->messageId != firstId);
}

void CoreTests::editUserMessageTruncatesAndRetags()
{
    const QString md = QStringLiteral(
        "```msg\n{\"id\":\"u1\",\"role\":\"user\"}\n```\n\n"
        "First question.\n\n"
        "```msg\n{\"id\":\"m1\",\"role\":\"assistant\"}\n```\n\n"
        "First answer.\n");

    be::DocumentStore store;
    store.loadMarkdown(md);
    QCOMPARE(store.blockCount(), qsizetype(2));

    // Truncate at the user message and re-add new text as a fresh user message.
    const qsizetype row = store.rowForMessage(QStringLiteral("u1"));
    QCOMPARE(row, qsizetype(0));
    store.deleteBlocks(row, store.blockCount() - row);
    QCOMPARE(store.blockCount(), qsizetype(0));

    const QString newId = store.appendMessageBlocks(be::MessageRole::User, QStringLiteral("Edited question."));
    QCOMPARE(store.blockCount(), qsizetype(1));
    QCOMPARE(store.blockAt(0)->role, be::MessageRole::User);
    QCOMPARE(store.blockAt(0)->messageId, newId);
    QCOMPARE(store.blockAt(0)->markdown(), QStringLiteral("Edited question."));
    QVERIFY(newId != QStringLiteral("u1"));
}

QTEST_MAIN(CoreTests)

#include "core_tests.moc"
