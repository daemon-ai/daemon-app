// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/autostart/autostart_controller.h"

#include "platform/autostart/autostart_backend.h"
#include "settings/isettings_store.h"

#include <QCoreApplication>

namespace autostart {

namespace {
// One-time first-run default marker (see applyFirstRunDefault).
const QString kConfiguredKey = QStringLiteral("app/autostartConfigured");
// The app version the entry was last registered under (macOS re-register).
const QString kRegisteredVersionKey = QStringLiteral("app/autostartRegisteredVersion");
} // namespace

AutostartController::AutostartController(QString program, settings::ISettingsStore* settings,
                                         QObject* parent)
    : QObject(parent), m_program(std::move(program)), m_settings(settings) {
    m_status = backend::query(m_program);
}

QString AutostartController::reason() const {
    if (m_status.state == State::RequiresApproval) {
        return tr("Waiting for your approval in System Settings › Login Items.");
    }
    switch (m_status.detail) {
    case Detail::NoProgram:
        return tr("The Daemon desktop app could not be found next to this client.");
    case Detail::NeedsAppBundle:
        return tr("Launch at login needs the installed Daemon app bundle.");
    case Detail::Translocated:
    case Detail::RunningFromDmg:
        return tr("Move Daemon to the Applications folder first, then relaunch it from there.");
    case Detail::Flatpak:
        return tr("Managed by your system's Flatpak permissions, not by Daemon.");
    case Detail::RegistrationFailed:
        return m_status.message.isEmpty()
                   ? tr("The system refused the login item.")
                   : tr("The system refused the login item: %1").arg(m_status.message);
    case Detail::None:
        return {};
    }
    return {};
}

void AutostartController::setEnabled(bool on) {
    if (!available() || (on && enabled()) || (!on && m_status.state == State::Disabled)) {
        return;
    }
    m_status = backend::apply(m_program, on);
    markConfigured();
    if (enabled() || requiresApproval()) {
        noteRegisteredVersion();
    }
    emit statusChanged();
}

void AutostartController::refresh() {
    m_status = backend::query(m_program);
    emit statusChanged();
}

void AutostartController::openApprovalSettings() {
    backend::openApprovalSettings();
}

void AutostartController::repairOnBoot() {
    if (!supported()) {
        return;
    }
    bool changed = backend::repair(m_program);
    // macOS: launchd caches the registered executable; after a SelfApply
    // bundle swap the entry must be re-registered once under the new version.
    if ((enabled() || requiresApproval()) && m_settings != nullptr) {
        const QString current = QCoreApplication::applicationVersion();
        if (m_settings->value(kRegisteredVersionKey).toString() != current) {
            backend::reregister(m_program);
            m_settings->setValue(kRegisteredVersionKey, current);
            changed = true;
        }
    }
    if (changed) {
        refresh();
    }
}

void AutostartController::applyFirstRunDefault() {
    if (m_settings == nullptr || m_settings->value(kConfiguredKey, false).toBool()) {
        return;
    }
    // One-shot regardless of outcome: a Blocked environment (e.g. running off
    // the DMG) is not retried behind the user's back on a later boot - the
    // Settings toggle stays the explicit path from then on.
    m_settings->setValue(kConfiguredKey, true);
    if (m_status.state != State::Disabled) {
        return;
    }
    m_status = backend::apply(m_program, true);
    if (enabled() || requiresApproval()) {
        noteRegisteredVersion();
    }
    emit statusChanged();
}

void AutostartController::markConfigured() {
    if (m_settings != nullptr) {
        m_settings->setValue(kConfiguredKey, true);
    }
}

void AutostartController::noteRegisteredVersion() {
    if (m_settings != nullptr) {
        m_settings->setValue(kRegisteredVersionKey, QCoreApplication::applicationVersion());
    }
}

} // namespace autostart
