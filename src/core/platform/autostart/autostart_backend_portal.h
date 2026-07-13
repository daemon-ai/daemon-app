// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "platform/autostart/autostart_status.h"

#include <QObject>
#include <QString>
#include <QVariantMap>

// Flatpak launch-at-login over the freedesktop Background portal
// (org.freedesktop.portal.Background.RequestBackground). Inside a Flatpak
// sandbox the XDG ~/.config/autostart write in autostart_backend_xdg.cpp only
// lands in the per-app sandbox config and is never seen by the host session, so
// the sandboxed backend delegates here: the portal writes a host-side autostart
// entry that relaunches the app via `flatpak run` at login. The portal is
// asynchronous (a Request object emits a Response signal), so this seam adapts
// it to the synchronous backend contract by pumping a scoped event loop.
namespace autostart::portal {

// Whether the Background portal is reachable on the session bus. False when
// there is no session bus or the xdg-desktop-portal Desktop service is absent;
// the sandboxed backend then reports the pre-portal "managed by Flatpak"
// Unsupported state as the documented fallback.
[[nodiscard]] bool available();

// Issue RequestBackground(autostart=enable) and block on the portal's Response.
// Returns Enabled/Disabled on a definitive grant/denial, Blocked +
// RegistrationFailed (carrying the D-Bus error text) when the call reached the
// portal but failed, or Unsupported + Flatpak when the portal is unreachable so
// the caller falls back to the pre-portal behavior.
[[nodiscard]] Status requestBackground(const QString& program, bool enable);

namespace detail {

// Slot receiver for the portal's async Request.Response signal. Kept in the
// header (not the .cpp) so AUTOMOC emits a standalone moc_*.cpp rather than an
// in-TU .moc include, keeping the generated meta-object out of the clang-tidy
// source glob. `response` follows the portal convention: 0 = success, 1 = user
// cancelled, 2 = ended some other way.
class ResponseWaiter : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

    [[nodiscard]] bool received() const { return m_received; }
    [[nodiscard]] uint response() const { return m_response; }
    [[nodiscard]] const QVariantMap& results() const { return m_results; }

public slots:
    void onResponse(uint response, const QVariantMap& results);

signals:
    void finished();

private:
    bool m_received = false;
    uint m_response = 2;
    QVariantMap m_results;
};

} // namespace detail

} // namespace autostart::portal
