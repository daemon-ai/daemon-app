// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Linux (and other freedesktop unixes): the XDG autostart directory, the one
// mechanism every DE implements (Plasma, GNOME via xdg-autostart-generator,
// XFCE, Cinnamon, MATE, LXQt, Budgie). The entry is visible and toggleable in
// the DEs' own autostart UIs; external disables (Hidden=true /
// X-GNOME-Autostart-enabled=false) read back as Disabled here and are
// preserved by repair().

#include "platform/autostart/autostart_backend.h"
#include "platform/autostart/autostart_backend_portal.h"
#include "platform/autostart/autostart_command.h"
#include "platform/autostart/autostart_entry.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>

namespace autostart::backend {

namespace {

QString entryPath() {
    // GenericConfigLocation = ~/.config (the spec's $XDG_CONFIG_HOME), and the
    // test-mode redirect QStandardPaths::setTestModeEnabled provides.
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
           QStringLiteral("/autostart/daemon-app.desktop");
}

QString readEntry(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

bool writeEntry(const QString& path, const QString& content, QString* error) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *error = file.errorString();
        return false;
    }
    file.write(content.toUtf8());
    if (!file.commit()) {
        *error = file.errorString();
        return false;
    }
    return true;
}

// The non-Flatpak environment gate. Flatpak is handled separately (it routes to
// the Background portal, autostart_backend_portal.h), so only the no-launchable-
// program case remains here.
bool noProgram(const QString& program, Status* status) {
    if (program.isEmpty()) {
        *status = {State::Unsupported, Detail::NoProgram, {}};
        return true;
    }
    return false;
}

// Inside a Flatpak sandbox the XDG autostart-dir write below never reaches the
// host session, so launch-at-login goes through the Background portal. When the
// portal is unreachable the query falls back to the pre-portal Unsupported/
// Flatpak status ("managed by your system's Flatpak permissions").
Status portalQuery() {
    // The Background portal offers no state read-back, so a live query cannot
    // reflect a prior grant: report a clean Disabled (toggleable) when the
    // portal is present so the settings row is shown and the user can (re)assert
    // the request; the portal echoes the real grant in apply()'s Response.
    return portal::available() ? Status{State::Disabled, Detail::None, {}}
                               : Status{State::Unsupported, Detail::Flatpak, {}};
}

} // namespace

Status query(const QString& program) {
    if (runningInsideFlatpak()) {
        return portalQuery();
    }
    Status gate;
    if (noProgram(program, &gate)) {
        return gate;
    }
    const QString path = entryPath();
    if (!QFile::exists(path)) {
        return {State::Disabled, Detail::None, {}};
    }
    const EntryFacts facts = parseAutostartEntry(readEntry(path));
    return {entryEnablesAutostart(facts) ? State::Enabled : State::Disabled, Detail::None, {}};
}

Status apply(const QString& program, bool enable) {
    if (runningInsideFlatpak()) {
        return portal::requestBackground(program, enable);
    }
    Status gate;
    if (noProgram(program, &gate)) {
        return gate;
    }
    const QString path = entryPath();
    if (!enable) {
        QFile::remove(path);
        return {State::Disabled, Detail::None, {}};
    }
    QString error;
    // A fresh enable always writes a clean, active entry (clearing any prior
    // Hidden / DE-disable state - the user explicitly asked for on).
    if (!writeEntry(path, renderAutostartEntry(program, {hiddenArg()}), &error)) {
        return {State::Blocked, Detail::RegistrationFailed, error};
    }
    return {State::Enabled, Detail::None, {}};
}

bool repair(const QString& program) {
    // The portal owns the host-side Flatpak autostart entry; nothing local to
    // rewrite (the sandbox never sees a stale ~/.config/autostart file).
    if (runningInsideFlatpak()) {
        return false;
    }
    Status gate;
    if (noProgram(program, &gate)) {
        return false;
    }
    const QString path = entryPath();
    if (!QFile::exists(path)) {
        return false;
    }
    const EntryFacts facts = parseAutostartEntry(readEntry(path));
    if (!entryNeedsRewrite(facts, program, hiddenArg())) {
        return false;
    }
    // Preserve the user's external disable flags across the rewrite.
    QString error;
    return writeEntry(
        path, renderAutostartEntry(program, {hiddenArg()}, facts.hidden, facts.gnomeEnabled),
        &error);
}

void reregister(const QString& /*program*/) {}

void openApprovalSettings() {}

} // namespace autostart::backend
