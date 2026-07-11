// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// tst_mirror_seeder (mirror A8) — the mock-mode single Writer (spec 09 §9, §5.1). Asserts:
//  (1) seed() writes every kind through the SAME apply/commit pipeline the ingestor uses, journal
//      origin = Seeder throughout, in ONE batch (§4.2);
//  (2) parity BY CONSTRUCTION — a lens (ConversationListModel) built over the seeded store renders
//      the seeded rows (same store, same journal, same lenses; §9);
//  (3) a scripted timeline (ScenarioPlayer) fires steps deterministically as the clock advances,
//      each a Seeder batch (message arrivals, presence flips) still stamped origin = Seeder;
//  (4) the verb script resolves ok / reject / timeout with an optional arg matcher — the MANDATORY
//      rejection fixtures (§6.5) exist and match by predicate.

#include "mirror/conversation_list_model.h"
#include "mirror/observe_coarse.h"
#include "mirror/seeder.h"
#include "mirror/store.h"

#include <QAbstractItemModelTester>
#include <QJsonObject>
#include <QtTest>
#include <vector>

using namespace mirror;

namespace {

Conversation conv(const QString& t, const QString& id, const QString& title) {
    Conversation c;
    c.transport = t;
    c.id = id;
    c.title = title;
    return c;
}

Contact contact(const QString& t, const QString& id, const QString& name) {
    Contact c;
    c.transport = t;
    c.id = id;
    c.display_name = name;
    return c;
}

Person person(const QString& id, const QString& alias) {
    Person p;
    p.id = id;
    p.alias = alias;
    return p;
}

ChatMessage msg(const QString& t, const QString& cv, quint64 cursor, const QString& text) {
    ChatMessage m;
    m.transport = t;
    m.conv = cv;
    m.cursor = cursor;
    m.text = text;
    return m;
}

SeedSet demoSeed() {
    SeedSet s;
    s.conversations = {conv("m", "!a", "Alpha"), conv("m", "!b", "Bravo")};
    s.contacts = {contact("m", "@u", "User")};
    s.persons = {person("p1", "Ada")};
    s.chat["m\x1f!a"] = {msg("m", "!a", 1, "hello"), msg("m", "!a", 2, "world")};
    return s;
}

} // namespace

class TstMirrorSeeder : public QObject {
    Q_OBJECT

private slots:
    // (1) + (3): seed + timeline write through the apply pipeline, journal origin = Seeder only.
    void seedAndTimelineStampSeederOrigin() {
        CoarseObserve observe;
        Store store(observe);
        Seeder seeder(store);

        Scenario scn;
        scn.seed = demoSeed();
        scn.apiVersion = 39;
        // Scripted timeline: a message arrives at t=100, a new conversation at t=200.
        scn.timeline.push_back(
            {100, [](Seeder& s) { s.appendMessage(msg("m", "!a", 3, "later")); }});
        scn.timeline.push_back(
            {200, [](Seeder& s) { s.upsertConversation(conv("m", "!c", "Charlie")); }});

        const ChatMessageScope scopeA{QStringLiteral("m"), QStringLiteral("!a")};
        ScenarioPlayer player(seeder, std::move(scn));
        player.playSeed();
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(2));
        QCOMPARE(store.snapshot().contacts.size(), std::size_t(1));
        QCOMPARE(store.snapshot().persons.size(), std::size_t(1));
        QVERIFY(store.snapshot().chat.count(scopeA));

        // Advance the clock: t=150 fires only the first step; t=250 fires the second.
        QCOMPARE(player.advanceTo(150), 1);
        QCOMPARE(store.snapshot().chat[scopeA].meta.item_count, 3);
        QVERIFY(!player.timelineExhausted());
        QCOMPARE(player.advanceTo(250), 1);
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(3));
        QVERIFY(player.timelineExhausted());

        // Every stamped record is origin = Seeder — no mock-only apply below the seam (§14 rule 5).
        for (const JournalRecord& rec : store.journal().records()) {
            QCOMPARE(rec.origin, JournalOrigin::Seeder);
        }
    }

    // (2) parity by construction: a lens over the seeded store renders the seeded rows.
    void lensRendersSeededRows() {
        CoarseObserve observe;
        Store store(observe);
        Seeder seeder(store);
        seeder.seed(demoSeed());

        ConversationListModel model(store);
        auto tester = std::make_unique<QAbstractItemModelTester>(&model);
        QCOMPARE(model.rowCount(), 2);
        // Presentation sort is title case-insensitive: Alpha, Bravo.
        QCOMPARE(model.data(model.index(0, 0), ConversationListModel::TitleRole).toString(),
                 QStringLiteral("Alpha"));
        QCOMPARE(model.data(model.index(1, 0), ConversationListModel::TitleRole).toString(),
                 QStringLiteral("Bravo"));

        // A later seeded conversation live-updates the same lens (delta path, no reset).
        seeder.upsertConversation(conv("m", "!c", "Aardvark"));
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(model.data(model.index(0, 0), ConversationListModel::TitleRole).toString(),
                 QStringLiteral("Aardvark"));
    }

    // (4) scripted verb outcomes incl. the MANDATORY rejection fixtures + arg matcher.
    void verbScriptResolvesOutcomes() {
        VerbScript script;
        script.add(VerbScript::ok(QStringLiteral("ConvSend"), /*echoDelayMs=*/20));
        // A rejection fixture keyed on an arg predicate (message == "boom").
        VerbRule boom = VerbScript::reject(QStringLiteral("ConvSend"), QStringLiteral("Forbidden"),
                                           QStringLiteral("blocked"));
        boom.matcher = [](const QJsonObject& args) {
            return args.value(QStringLiteral("message")).toString() == QStringLiteral("boom");
        };
        // Insert the more specific rule FIRST so it wins over the catch-all ok.
        VerbScript ordered;
        ordered.add(boom);
        ordered.add(VerbScript::ok(QStringLiteral("ConvSend"), 20));
        ordered.add(VerbScript::timeout(QStringLiteral("TurnPrompt"), 5000));

        QJsonObject boomArgs{{QStringLiteral("message"), QStringLiteral("boom")}};
        QJsonObject okArgs{{QStringLiteral("message"), QStringLiteral("hi")}};

        const VerbOutcome rej = ordered.outcomeFor(QStringLiteral("ConvSend"), boomArgs);
        QCOMPARE(rej.kind, VerbOutcomeKind::Reject);
        QCOMPARE(rej.errorKind, QStringLiteral("Forbidden"));

        const VerbOutcome okOut = ordered.outcomeFor(QStringLiteral("ConvSend"), okArgs);
        QCOMPARE(okOut.kind, VerbOutcomeKind::Ok);
        QCOMPARE(okOut.echoDelayMs, 20);

        const VerbOutcome to = ordered.outcomeFor(QStringLiteral("TurnPrompt"), okArgs);
        QCOMPARE(to.kind, VerbOutcomeKind::Timeout);

        // An unscripted verb defaults to Ok (mock keeps working, §9).
        const VerbOutcome def = ordered.outcomeFor(QStringLiteral("RosterAdd"), okArgs);
        QCOMPARE(def.kind, VerbOutcomeKind::Ok);
    }
};

QTEST_GUILESS_MAIN(TstMirrorSeeder)
#include "tst_mirror_seeder.moc"
