// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>

class QTcpServer;

namespace auth {

// The desktop redirect-capture sink for interactive auth (daemon-interactive-auth-spec.md §8): a
// short-lived loopback HTTP listener on 127.0.0.1:0 whose URL rides AuthBegin.redirect_uri. The
// homeserver/IdP redirects the user's browser here; the sink answers with a tiny "return to the
// app" page, emits the captured query string, and shuts down. One capture per start() — the flow
// id it feeds is single-use anyway.
//
// Platforms without a listenable loopback (wasm; locked-down sandboxes) simply fail start(); the
// AuthFlowController then runs URL-only (manual callback paste), so no consumer needs a platform
// fork. Nothing here is a domain decision — the node completes the exchange; this only relays.
class RedirectSink : public QObject {
    Q_OBJECT

public:
    explicit RedirectSink(QObject* parent = nullptr);
    ~RedirectSink() override;

    // Start listening on an ephemeral loopback port. Returns false when listening is impossible
    // (wasm / no loopback); the caller falls back to manual paste. Idempotent while listening.
    bool start();
    // Tear the listener down (idempotent; also runs on destruction and after a capture).
    void stop();

    [[nodiscard]] bool listening() const;
    // The redirect URI to hand to AuthBegin (e.g. "http://127.0.0.1:53127/cb"); empty until
    // start() succeeds.
    [[nodiscard]] QString redirectUri() const { return m_redirectUri; }

signals:
    // The browser hit the sink: `callback` is the raw request query string (e.g.
    // "loginToken=..." or "code=...&state=..."), exactly what AuthComplete.callback wants.
    void captured(const QString& callback);

private:
    void onNewConnection();

    QTcpServer* m_server = nullptr;
    QString m_redirectUri;
};

} // namespace auth
