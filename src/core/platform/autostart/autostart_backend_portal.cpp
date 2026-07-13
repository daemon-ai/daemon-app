// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/autostart/autostart_backend_portal.h"

#include "platform/autostart/autostart_command.h"

#include <QDateTime>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QEventLoop>
#include <QFileInfo>
#include <QStringList>
#include <QTimer>
#include <QVariant>

namespace autostart::portal {

namespace {

const QString kService = QStringLiteral("org.freedesktop.portal.Desktop");
const QString kObjectPath = QStringLiteral("/org/freedesktop/portal/desktop");
const QString kBackgroundIface = QStringLiteral("org.freedesktop.portal.Background");
const QString kRequestIface = QStringLiteral("org.freedesktop.portal.Request");
const QString kResponseSignal = QStringLiteral("Response");

// The portal can pop an interactive permission dialog; give the user room to
// answer before giving up (and reporting Blocked so the UI can retry).
constexpr int kResponseTimeoutMs = 20000;

// The command the host-side autostart entry runs. Inside the sandbox the portal
// prefixes `flatpak run <app-id>`, so only the trailing arguments matter here:
// launch minimized to the tray, matching every other autostart lane.
QStringList autostartCommandline(const QString& program) {
    QString leaf = QFileInfo(program).fileName();
    if (leaf.isEmpty()) {
        leaf = QStringLiteral("daemon-app");
    }
    return {leaf, hiddenArg()};
}

// The Request object path the portal will reuse for this call, per the portal
// spec: /org/freedesktop/portal/desktop/request/<SENDER>/<token>, where SENDER
// is the unique bus name with the leading ':' dropped and '.' -> '_'. Connecting
// to it BEFORE issuing the call closes the race where the Response fires before
// the reply handle is observed.
QString predictedRequestPath(const QDBusConnection& bus, const QString& token) {
    QString sender = bus.baseService();
    if (sender.startsWith(u':')) {
        sender.remove(0, 1);
    }
    sender.replace(u'.', u'_');
    return QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, token);
}

} // namespace

namespace detail {

void ResponseWaiter::onResponse(uint response, const QVariantMap& results) {
    m_received = true;
    m_response = response;
    m_results = results;
    emit finished();
}

} // namespace detail

bool available() {
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return false;
    }
    QDBusConnectionInterface* iface = bus.interface();
    return iface != nullptr && iface->isServiceRegistered(kService).value();
}

Status requestBackground(const QString& program, bool enable) {
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected() || !available()) {
        // No usable portal: fall back to the pre-portal "managed by Flatpak".
        return {State::Unsupported, Detail::Flatpak, {}};
    }

    // A per-call token so the predicted Request path is unique.
    const QString token = QStringLiteral("daemon_autostart_%1")
                              .arg(QString::number(QDateTime::currentMSecsSinceEpoch(), 16));
    const QString predicted = predictedRequestPath(bus, token);

    detail::ResponseWaiter waiter;
    QEventLoop loop;
    QObject::connect(&waiter, &detail::ResponseWaiter::finished, &loop, &QEventLoop::quit);
    bus.connect(kService, predicted, kRequestIface, kResponseSignal, &waiter,
                SLOT(onResponse(uint, QVariantMap)));

    QVariantMap options;
    options.insert(QStringLiteral("handle_token"), token);
    options.insert(QStringLiteral("reason"),
                   QStringLiteral("Launch Daemon at login, minimized to the tray."));
    options.insert(QStringLiteral("autostart"), enable);
    options.insert(QStringLiteral("commandline"), autostartCommandline(program));
    options.insert(QStringLiteral("dbus-activatable"), false);

    QDBusMessage call = QDBusMessage::createMethodCall(kService, kObjectPath, kBackgroundIface,
                                                       QStringLiteral("RequestBackground"));
    call << QString() << options;
    const QDBusReply<QDBusObjectPath> reply = bus.call(call);
    if (!reply.isValid()) {
        return {State::Blocked, Detail::RegistrationFailed, reply.error().message()};
    }

    // Reconnect if the portal chose a different object path than predicted (older
    // xdg-desktop-portal builds).
    const QString actual = reply.value().path();
    if (actual != predicted) {
        bus.disconnect(kService, predicted, kRequestIface, kResponseSignal, &waiter,
                       SLOT(onResponse(uint, QVariantMap)));
        bus.connect(kService, actual, kRequestIface, kResponseSignal, &waiter,
                    SLOT(onResponse(uint, QVariantMap)));
    }

    if (!waiter.received()) {
        QTimer::singleShot(kResponseTimeoutMs, &loop, &QEventLoop::quit);
        loop.exec();
    }

    if (!waiter.received()) {
        return {State::Blocked, Detail::RegistrationFailed,
                QStringLiteral("the Background portal did not respond")};
    }
    if (waiter.response() != 0) {
        return {State::Blocked, Detail::RegistrationFailed,
                QStringLiteral("the Background request was dismissed")};
    }
    const bool granted = waiter.results().value(QStringLiteral("autostart"), enable).toBool();
    return {granted ? State::Enabled : State::Disabled, Detail::None, {}};
}

} // namespace autostart::portal
