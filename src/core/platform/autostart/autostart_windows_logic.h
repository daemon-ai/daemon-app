// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <cstdint>
#include <QByteArray>
#include <QString>

// Pure Windows autostart logic (HKCU Run value composition + the Explorer
// StartupApproved byte parse), shared by the Windows backend and the unit
// tests. Compiled on every platform so Linux CI covers it; only
// autostart_backend_win.cpp touches the registry.
namespace autostart {

// The `Run` value data: `"<native program path>" --hidden`.
[[nodiscard]] QString composeRunValue(const QString& nativeProgramPath, const QString& hiddenArg);

// The program path back out of a Run value (quoted or bare); empty when the
// value is blank.
[[nodiscard]] QString runValueProgram(const QString& runValue);

// Explorer's StartupApproved\Run verdict for an entry: a 12-byte REG_BINARY
// blob whose FIRST byte is even when the entry may run and odd when the user
// disabled it in Task Manager / Settings > Apps > Startup (observed values:
// 0x02/0x06 enabled, 0x03/0x07 disabled). No blob at all means "never
// touched", which also runs.
enum class ApprovedState : std::uint8_t { NoData, Enabled, Disabled };

[[nodiscard]] ApprovedState decodeStartupApproved(const QByteArray& blob);

} // namespace autostart
