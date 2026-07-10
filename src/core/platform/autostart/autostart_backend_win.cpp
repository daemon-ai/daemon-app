// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Windows: the per-user HKCU Run key (no elevation, matching the
// RequestExecutionLevel-user NSIS lane), with Explorer's StartupApproved key
// consulted for the Task Manager / Settings > Startup-apps disable state and
// cleared on re-enable so a prior disable does not mask the new entry.

#include "platform/autostart/autostart_backend.h"
#include "platform/autostart/autostart_command.h"
#include "platform/autostart/autostart_windows_logic.h"

#include <QDir>
#include <QSettings>

namespace autostart::backend {

namespace {

const QString kValueName = QStringLiteral("Daemon");

QString runKeyPath() {
    return QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
}

QString approvedKeyPath() {
    return QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\"
                          "Explorer\\StartupApproved\\Run");
}

} // namespace

Status query(const QString& program) {
    if (program.isEmpty()) {
        return {State::Unsupported, Detail::NoProgram, {}};
    }
    QSettings run(runKeyPath(), QSettings::NativeFormat);
    if (!run.contains(kValueName)) {
        return {State::Disabled, Detail::None, {}};
    }
    QSettings approved(approvedKeyPath(), QSettings::NativeFormat);
    const ApprovedState verdict = decodeStartupApproved(approved.value(kValueName).toByteArray());
    return {
        verdict == ApprovedState::Disabled ? State::Disabled : State::Enabled, Detail::None, {}};
}

Status apply(const QString& program, bool enable) {
    if (program.isEmpty()) {
        return {State::Unsupported, Detail::NoProgram, {}};
    }
    QSettings run(runKeyPath(), QSettings::NativeFormat);
    QSettings approved(approvedKeyPath(), QSettings::NativeFormat);
    if (enable) {
        run.setValue(kValueName, composeRunValue(QDir::toNativeSeparators(program), hiddenArg()));
        // Clear a prior Task-Manager disable so the entry actually runs again.
        approved.remove(kValueName);
    } else {
        run.remove(kValueName);
        approved.remove(kValueName);
    }
    run.sync();
    approved.sync();
    if (run.status() != QSettings::NoError) {
        return {State::Blocked, Detail::RegistrationFailed,
                QStringLiteral("registry write failed")};
    }
    return {enable ? State::Enabled : State::Disabled, Detail::None, {}};
}

bool repair(const QString& program) {
    if (program.isEmpty()) {
        return false;
    }
    QSettings run(runKeyPath(), QSettings::NativeFormat);
    if (!run.contains(kValueName)) {
        return false;
    }
    const QString native = QDir::toNativeSeparators(program);
    const QString existing = run.value(kValueName).toString();
    if (runValueProgram(existing) == native && existing.contains(hiddenArg())) {
        return false;
    }
    // Rewrite only the command; the StartupApproved verdict (the user's
    // enable/disable choice) is deliberately left untouched.
    run.setValue(kValueName, composeRunValue(native, hiddenArg()));
    run.sync();
    return run.status() == QSettings::NoError;
}

void reregister(const QString& /*program*/) {}

void openApprovalSettings() {}

} // namespace autostart::backend
