#include "status_bar_model.h"

#include <QtTest>

// Exercises the shared StatusBarModel's formatting (the C++ port of the JS funcs
// in StatusBar.qml): elapsed-time formatting, token abbreviation, the context
// percentage, and the unicode context bar. These guard byte-for-byte parity with
// the QML the GUI footer used to render.
class TestStatusModel : public QObject {
    Q_OBJECT

private slots:
    // formatElapsed mirrors the QML: "0:00" for an unset start, m:ss under an
    // hour, and h:mm:ss past it, measured against the model's nowMs.
    void formatElapsedMatchesQml()
    {
        // Capture the baseline BEFORE constructing so the model's internal nowMs
        // (seeded at construction) is >= now. Feeding start = now - N then yields
        // an elapsed of N + epsilon, which floors to the intended whole second
        // (avoids an off-by-one when nowMs is a few ms ahead of `now`).
        const double now = QDateTime::currentMSecsSinceEpoch();
        StatusBarModel model;

        QCOMPARE(model.formatElapsed(0), QStringLiteral("0:00"));
        QCOMPARE(model.formatElapsed(now), QStringLiteral("0:00"));
        QCOMPARE(model.formatElapsed(now - 5'000), QStringLiteral("0:05"));
        QCOMPARE(model.formatElapsed(now - 65'000), QStringLiteral("1:05"));
        QCOMPARE(model.formatElapsed(now - 3'661'000), QStringLiteral("1:01:01"));
    }

    // abbrev mirrors the QML: k/M suffixes with a trailing ".0" stripped.
    void abbrevMatchesQml()
    {
        StatusBarModel model;
        QCOMPARE(model.abbrev(0), QStringLiteral("0"));
        QCOMPARE(model.abbrev(999), QStringLiteral("999"));
        QCOMPARE(model.abbrev(1000), QStringLiteral("1k"));
        QCOMPARE(model.abbrev(12500), QStringLiteral("12.5k"));
        QCOMPARE(model.abbrev(128000), QStringLiteral("128k"));
        QCOMPARE(model.abbrev(1'000'000), QStringLiteral("1M"));
        QCOMPARE(model.abbrev(1'500'000), QStringLiteral("1.5M"));
    }

    // contextPercent rounds used/max to a whole percent.
    void contextPercentRounds()
    {
        StatusBarModel model;
        // Defaults: 12500 / 128000 ~= 9.77% -> 10.
        QCOMPARE(model.contextPercent(), 10);
        model.setContextUsed(64000);
        model.setContextMax(128000);
        QCOMPARE(model.contextPercent(), 50);
        model.setContextMax(0);
        QCOMPARE(model.contextPercent(), 0);
    }

    // contextBar renders a 10-cell filled/empty unicode meter with the percent.
    void contextBarMatchesQml()
    {
        StatusBarModel model;
        model.setContextUsed(50);
        model.setContextMax(100);
        QCOMPARE(model.contextBar(), QStringLiteral("[\u2588\u2588\u2588\u2588\u2588"
                                                    "\u2591\u2591\u2591\u2591\u2591] 50%"));
        model.setContextUsed(0);
        QCOMPARE(model.contextBar(), QStringLiteral("[\u2591\u2591\u2591\u2591\u2591"
                                                    "\u2591\u2591\u2591\u2591\u2591] 0%"));
        model.setContextUsed(100);
        QCOMPARE(model.contextBar(), QStringLiteral("[\u2588\u2588\u2588\u2588\u2588"
                                                    "\u2588\u2588\u2588\u2588\u2588] 100%"));
    }

    // contextLabel abbreviates used/max (or "used tok" when max is unknown).
    void contextLabelMatchesQml()
    {
        StatusBarModel model;
        model.setContextUsed(12500);
        model.setContextMax(128000);
        QCOMPARE(model.contextLabel(), QStringLiteral("12.5k/128k"));
        model.setContextMax(0);
        QCOMPARE(model.contextLabel(), QStringLiteral("12.5k tok"));
    }

    // Gateway tone maps the placeholder states to default/warning/danger.
    void gatewayToneMapsStates()
    {
        StatusBarModel model;
        QCOMPARE(model.gatewayTone(), QStringLiteral("default"));
        model.setGatewayState(QStringLiteral("connecting"));
        QVERIFY(model.gatewayDegraded());
        QCOMPARE(model.gatewayTone(), QStringLiteral("warning"));
        model.setGatewayState(QStringLiteral("offline"));
        QVERIFY(model.gatewayOffline());
        QCOMPARE(model.gatewayTone(), QStringLiteral("danger"));
    }

    // agentsDetail prefers the failed count, falls back to running, else empty.
    void agentsDetailPrefersFailed()
    {
        StatusBarModel model;
        QVERIFY(model.agentsDetail().isEmpty());
        model.setAgentsRunning(3);
        QCOMPARE(model.agentsDetail(), QStringLiteral("3 running"));
        model.setAgentsFailed(1);
        QCOMPARE(model.agentsDetail(), QStringLiteral("1 failed"));
    }

    // rateLabel is empty until a rateLimit window is known, then "remaining/limit".
    // applyTurnEvents ingests the same absolute rateLimit shape the daemon emits.
    void rateLabelFromTurnEvents()
    {
        StatusBarModel model;
        QVERIFY(model.rateLabel().isEmpty()); // no window yet

        QSignalSpy spy(&model, &StatusBarModel::rateChanged);
        const QVariantList events { QVariantMap {
            { QStringLiteral("type"), QStringLiteral("rateLimit") },
            { QStringLiteral("remaining"), 74 },
            { QStringLiteral("limit"), 80 },
        } };
        model.applyTurnEvents(events);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(model.rateRemaining(), 74);
        QCOMPARE(model.rateLimit(), 80);
        QCOMPARE(model.rateLabel(), QStringLiteral("74/80"));
    }

    // Gateway dropdown content starts empty/honest and is settable, emitting
    // gatewayInfoChanged when a daemon or demo provider supplies details.
    void gatewayInfoDefaultsAndSettable()
    {
        StatusBarModel model;
        QCOMPARE(model.gatewayConnectionText(), QStringLiteral("No daemon connection"));
        QVERIFY(model.gatewayLog().isEmpty());
        QVERIFY(model.gatewayPlatforms().isEmpty());

        QSignalSpy spy(&model, &StatusBarModel::gatewayInfoChanged);
        model.setGatewayConnectionText(QStringLiteral("Reconnecting\u2026"));
        QCOMPARE(model.gatewayConnectionText(), QStringLiteral("Reconnecting\u2026"));
        model.setGatewayLog({ QStringLiteral("gateway: down") });
        QCOMPARE(model.gatewayLog().size(), 1);
        QCOMPARE(spy.count(), 2);
    }
};

QTEST_GUILESS_MAIN(TestStatusModel)
#include "tst_status_model.moc"
