// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemonnet/mock_daemonnet.h"
#include "daemonnet/routing_dtos.h"
#include "domain/delivery.h"
#include "domain/ids.h"
#include "domain/origin.h"

#include <QtTest/QtTest>

using daemonnet::DecidedBy;
using daemonnet::MockDaemonNet;
using daemonnet::Resolution;
using domain::DeliveryTarget;
using domain::Origin;
using domain::OriginScopeKind;
using domain::ProfileRef;
using domain::RouteAddr;
using domain::SessionId;
using domain::SinkKind;
using domain::TransportId;

// The routing-manager model (routing-manager-design.md): resolve() precedence + the pin/handover/
// bindAccount mutations + the projections, all over the one MockDaemonNet seed.
class TestRoutingModel : public QObject {
    Q_OBJECT

    static Origin grp(const char* t, const char* chat) {
        Origin o;
        o.transport = TransportId(QString::fromLatin1(t));
        o.scope.kind = OriginScopeKind::Group;
        o.scope.chat = QString::fromLatin1(chat);
        return o;
    }
    static Origin dm(const char* t, const char* user) {
        Origin o;
        o.transport = TransportId(QString::fromLatin1(t));
        o.scope.kind = OriginScopeKind::Dm;
        o.scope.user = QString::fromLatin1(user);
        return o;
    }

private slots:
    // A seeded pin resolves first: session + agent come from the pin.
    void pinResolvesFirst() {
        MockDaemonNet net;
        const Resolution r = net.resolve(grp("matrix/@bot:hs.org", "#secops"));
        QCOMPARE(r.decidedBy, DecidedBy::Pin);
        QCOMPARE(r.session.toString(), QStringLiteral("s-secops"));
        QCOMPARE(r.profile.toString(), QStringLiteral("prof-2"));
    }

    // An unpinned chat matching the #secops* rule is decided by the rule's profile.
    void ruleDecidesUnpinned() {
        MockDaemonNet net;
        const Resolution r = net.resolve(grp("matrix/@bot:hs.org", "#secops-2"));
        QCOMPARE(r.decidedBy, DecidedBy::Rule);
        QCOMPARE(r.profile.toString(), QStringLiteral("prof-3"));
    }

    // A chat on a bound account that matches no profile-bearing rule falls to the account baseline.
    void accountBoundFallback() {
        MockDaemonNet net;
        const Resolution r = net.resolve(grp("matrix/@bot:hs.org", "#general"));
        QCOMPARE(r.decidedBy, DecidedBy::AccountBound);
        QCOMPARE(r.profile.toString(), QStringLiteral("prof-1"));
    }

    // An origin on an unbound transport falls to the node default.
    void nodeDefaultFallback() {
        MockDaemonNet net;
        const Resolution r = net.resolve(dm("slack/@bot", "@x:hs.org"));
        QCOMPARE(r.decidedBy, DecidedBy::Default);
        QCOMPARE(r.profile.toString(), QStringLiteral("prof-1"));
    }

    // bindChat adds a pin (resolve-first), unbindChat removes it.
    void bindAndUnbindChat() {
        MockDaemonNet net;
        const int before = net.routes().size();
        const Origin o = grp("matrix/@ops:hs.org", "#ops");
        QVERIFY(net.resolve(o).decidedBy != DecidedBy::Pin);

        net.bindChat(o, SessionId(QStringLiteral("s-acme")), ProfileRef(QStringLiteral("prof-1")));
        QCOMPARE(net.routes().size(), before + 1);
        const Resolution r = net.resolve(o);
        QCOMPARE(r.decidedBy, DecidedBy::Pin);
        QCOMPARE(r.session.toString(), QStringLiteral("s-acme"));

        net.unbindChat(o);
        QCOMPARE(net.routes().size(), before);
        QVERIFY(net.resolve(o).decidedBy != DecidedBy::Pin);
    }

    // Handover re-points the Primary; the prior Primary demotes to Spectator (exactly one Primary).
    void handoverRepointsPrimary() {
        MockDaemonNet net;
        const SessionId design(QStringLiteral("s-design"));
        DeliveryTarget gui;
        gui.transport = TransportId(QStringLiteral("gui"));
        gui.route = RouteAddr(QStringLiteral("desktop"));
        gui.kind = SinkKind::Primary;

        net.handover(design, gui);

        int primaries = 0;
        DeliveryTarget primary;
        for (const DeliveryTarget& t : net.deliveryTargets(design)) {
            if (t.kind == SinkKind::Primary) {
                ++primaries;
                primary = t;
            }
        }
        QCOMPARE(primaries, 1);
        QCOMPARE(primary.transport.toString(), QStringLiteral("gui"));
        QCOMPARE(primary.route.toString(), QStringLiteral("desktop"));
    }

    // bindAccount changes the account->agent baseline (and thus AccountBound resolution).
    void bindAccountChangesBaseline() {
        MockDaemonNet net;
        net.bindAccount(TransportId(QStringLiteral("slack/@bot")),
                        ProfileRef(QStringLiteral("prof-2")));
        const Resolution r = net.resolve(dm("slack/@bot", "@x:hs.org"));
        QCOMPARE(r.decidedBy, DecidedBy::AccountBound);
        QCOMPARE(r.profile.toString(), QStringLiteral("prof-2"));
    }

    // The projections expose the seeded state.
    void projectionsExposeSeed() {
        MockDaemonNet net;
        QVERIFY(net.routes().size() >= 2);         // the seeded pins
        QVERIFY(net.bindingRules().size() >= 1);   // the config rules
        QVERIFY(net.accountsAgents().size() >= 1); // instance_profiles
        // s-design has a Primary + a Spectator in the seed.
        const auto d = net.deliveryTargets(SessionId(QStringLiteral("s-design")));
        QVERIFY(d.size() >= 2);
    }
};

QTEST_MAIN(TestRoutingModel)

#include "tst_routing_model.moc"
