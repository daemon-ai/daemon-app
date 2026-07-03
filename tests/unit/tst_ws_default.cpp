// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "settings/ws_default.h"

#include <QtTest/QtTest>

using settings::deriveWsDefault;

// Guards the browser connection-target derivation (pure half of the wasm page-origin
// default): the `?ws=` query override wins, a ws-shaped saved target beats the page-origin
// default, and a single-origin page derives `ws(s)://<host[:port]>/ws` - the path contract
// with the daemon-node single-origin listener.
class TestWsDefault : public QObject {
    Q_OBJECT

private slots:
    // Single-origin default: same host AND port as the page, path /ws, scheme mapped pairwise
    // (http -> ws, https -> wss).
    void pageOriginDerivesWsUrl() {
        QCOMPARE(deriveWsDefault(QUrl(QStringLiteral("http://127.0.0.1:8977/daemon-app.html"))),
                 QStringLiteral("ws://127.0.0.1:8977/ws"));
        QCOMPARE(deriveWsDefault(QUrl(QStringLiteral("https://node.example/daemon-app.html"))),
                 QStringLiteral("wss://node.example/ws"));
        QCOMPARE(deriveWsDefault(QUrl(QStringLiteral("https://node.example:8443/app/index.html"))),
                 QStringLiteral("wss://node.example:8443/ws"));
    }

    // `?ws=` in the page query is an explicit instruction, so it wins over everything -
    // including a persisted target - and survives percent-encoding and sibling params.
    void queryOverrideWinsOverSaved() {
        const QUrl page(
            QStringLiteral("http://127.0.0.1:8977/daemon-app.html?ws=ws://10.0.0.5:1/"));
        QCOMPARE(deriveWsDefault(page, QStringLiteral("wss://saved.example/ws")),
                 QStringLiteral("ws://10.0.0.5:1/"));

        const QUrl encoded(
            QStringLiteral("http://127.0.0.1:8977/"
                           "daemon-app.html?theme=dark&ws=wss%3A%2F%2Fnode.example%3A9443%2Fws"));
        QCOMPARE(deriveWsDefault(encoded), QStringLiteral("wss://node.example:9443/ws"));
    }

    // A malformed override (non-ws scheme, no host) is ignored rather than surfaced: the
    // prefill falls back to the next tier instead of planting an unusable target.
    void malformedOverrideFallsThrough() {
        QCOMPARE(deriveWsDefault(
                     QUrl(QStringLiteral("http://127.0.0.1:8977/x.html?ws=http://node.example"))),
                 QStringLiteral("ws://127.0.0.1:8977/ws"));
        QCOMPARE(deriveWsDefault(QUrl(QStringLiteral("http://127.0.0.1:8977/x.html?ws=ws://"))),
                 QStringLiteral("ws://127.0.0.1:8977/ws"));
        QCOMPARE(deriveWsDefault(QUrl(QStringLiteral("http://127.0.0.1:8977/x.html?ws="))),
                 QStringLiteral("ws://127.0.0.1:8977/ws"));
    }

    // Desktop parity: a persisted (ws-shaped) target beats the platform default.
    void savedWsTargetBeatsPageOrigin() {
        QCOMPARE(deriveWsDefault(QUrl(QStringLiteral("http://127.0.0.1:8977/daemon-app.html")),
                                 QStringLiteral("wss://node.example:9443/ws")),
                 QStringLiteral("wss://node.example:9443/ws"));
    }

    // A saved socket path or bare host:port is unusable in a browser and must never leak into
    // the prefill; the page-origin default applies instead.
    void nonWsSavedTargetIsIgnored() {
        QCOMPARE(deriveWsDefault(QUrl(QStringLiteral("http://127.0.0.1:8977/daemon-app.html")),
                                 QStringLiteral("/run/user/1000/daemon/daemon.sock")),
                 QStringLiteral("ws://127.0.0.1:8977/ws"));
        QCOMPARE(deriveWsDefault(QUrl(QStringLiteral("http://127.0.0.1:8977/daemon-app.html")),
                                 QStringLiteral("node.example:8443")),
                 QStringLiteral("ws://127.0.0.1:8977/ws"));
    }

    // Non-http(s) pages (file://, empty) have no origin to derive from: empty result, so the
    // caller falls through to its platform fallback chain.
    void nonHttpPageYieldsEmpty() {
        QCOMPARE(deriveWsDefault(QUrl(QStringLiteral("file:///tmp/daemon-app.html"))), QString());
        QCOMPARE(deriveWsDefault(QUrl()), QString());
        // ... unless the override says otherwise (it is scheme-independent).
        QCOMPARE(deriveWsDefault(QUrl(QStringLiteral("file:///tmp/x.html?ws=ws://127.0.0.1:1/"))),
                 QStringLiteral("ws://127.0.0.1:1/"));
    }
};

QTEST_MAIN(TestWsDefault)
#include "tst_ws_default.moc"
