// tst_chat_conversation_mirror (mirror A5) — the chat controller's MIRROR read path (spec 09
// §8.1/§8.4, §5.8, §4.6). With a MirrorService bound, open() builds the ChatWindowModel lens and
// only DECLARES visibility (the ingestor issues the top-up); journal deltas re-project the shared
// markdown; demand paging mediates olderRequested → Ingestor::requestOlder and surfaces
// end-of-history; the legacy IChatService rows are ignored while the mirror is active.

#include "chat_conversation_controller.h"
#include "mirror/fetch_job.h"
#include "mirror/fetch_scheduler.h"
#include "mirror/mirror_service.h"

#include <QObject>
#include <QSignalSpy>
#include <QTest>
#include <vector>

using namespace mirror;

namespace {

ChatMessage chat(const QString& t, const QString& c, quint64 cursor, const QString& author,
                 const QString& text) {
    ChatMessage m;
    m.transport = t;
    m.conv = c;
    m.cursor = cursor;
    m.author = author;
    m.text = text;
    return m;
}

class RecordingExecutor : public FetchExecutor {
public:
    void execute(const FetchJob& job) override { executed.push_back(job); }
    [[nodiscard]] int count(FetchOp op) const {
        int n = 0;
        for (const FetchJob& j : executed) {
            if (j.op == op) {
                ++n;
            }
        }
        return n;
    }
    [[nodiscard]] FetchJob last(FetchOp op) const {
        for (auto it = executed.rbegin(); it != executed.rend(); ++it) {
            if (it->op == op) {
                return *it;
            }
        }
        return {};
    }
    std::vector<FetchJob> executed;
};

} // namespace

class TstChatConversationMirror : public QObject {
    Q_OBJECT

private slots:
    void openDeclaresVisibilityAndIngestorTopsUp() {
        MirrorService svc;
        svc.openInMemory();
        RecordingExecutor exec;
        svc.setFetchExecutor(&exec);

        ChatConversationController c;
        c.setMirror(&svc);
        c.open(QStringLiteral("m"), QStringLiteral("!room"));

        // The controller declared visibility; the INGESTOR issued the ConvHistory top-up (§5.8).
        QCOMPARE(exec.count(FetchOp::ConvHistory), 1);
        QCOMPARE(exec.last(FetchOp::ConvHistory).scope, QStringLiteral("m\x1f!room"));
        QVERIFY(c.timelineModel() != nullptr);
    }

    void journalDeltasReprojectMarkdown() {
        MirrorService svc;
        svc.openInMemory();
        ChatConversationController c;
        c.setMirror(&svc);
        c.open(QStringLiteral("m"), QStringLiteral("!room"));
        QVERIFY(c.empty());

        QSignalSpy md(&c, &ChatConversationController::markdownChanged);
        svc.ingestor().deliverChatPage(
            QStringLiteral("m"), QStringLiteral("!room"),
            {chat("m", "!room", 1, "alice", "hello"), chat("m", "!room", 2, "bob", "there")},
            /*nextCursor=*/2, /*headCursor=*/2);
        QVERIFY(md.count() >= 1);
        QCOMPARE(c.messageCount(), 2);
        QVERIFY(c.markdown().contains(QStringLiteral("**alice**")));
        QVERIFY(c.markdown().contains(QStringLiteral("there")));

        // A background conversation's delivery does NOT touch this controller's projection.
        const QString before = c.markdown();
        svc.ingestor().deliverChatPage(QStringLiteral("m"), QStringLiteral("!other"),
                                       {chat("m", "!other", 1, "zoe", "elsewhere")},
                                       /*nextCursor=*/1, /*headCursor=*/1);
        QCOMPARE(c.markdown(), before);
        QCOMPARE(c.messageCount(), 2);
    }

    void legacyRowsIgnoredWhileMirrorActive() {
        MirrorService svc;
        svc.openInMemory();
        ChatConversationController c;
        c.setMirror(&svc);
        c.open(QStringLiteral("m"), QStringLiteral("!room"));
        svc.ingestor().deliverChatPage(QStringLiteral("m"), QStringLiteral("!room"),
                                       {chat("m", "!room", 1, "alice", "mirror truth")},
                                       /*nextCursor=*/1, /*headCursor=*/1);
        // The mirror window is the read path (R2 discipline): messageCount comes from the lens,
        // and the markdown carries the canonical entity fields.
        QCOMPARE(c.messageCount(), 1);
        QVERIFY(c.markdown().contains(QStringLiteral("mirror truth")));
    }

    void loadEarlierSurfacesEndOfHistory() {
        MirrorService svc;
        svc.openInMemory();
        ChatConversationController c;
        c.setMirror(&svc);
        c.open(QStringLiteral("m"), QStringLiteral("!room"));
        svc.ingestor().deliverChatPage(QStringLiteral("m"), QStringLiteral("!room"),
                                       {chat("m", "!room", 1, "alice", "hello")},
                                       /*nextCursor=*/1, /*headCursor=*/1);
        QVERIFY(c.canLoadEarlier()); // not yet proven ended
        QSignalSpy spy(&c, &ChatConversationController::canLoadEarlierChanged);
        c.loadEarlier();
        // v38: older-than-window history is not expressible (Ingestor::requestOlder false) — the
        // mediation flips has-more off and the property change surfaces (§4.6 interim).
        QVERIFY(!c.canLoadEarlier());
        QVERIFY(spy.count() >= 1);
    }
};

QTEST_GUILESS_MAIN(TstChatConversationMirror)
#include "tst_chat_conversation_mirror.moc"
