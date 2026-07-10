// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/autostart/autostart_windows_logic.h"

namespace autostart {

QString composeRunValue(const QString& nativeProgramPath, const QString& hiddenArg) {
    return QLatin1Char('"') + nativeProgramPath + QLatin1Char('"') + QLatin1Char(' ') + hiddenArg;
}

QString runValueProgram(const QString& runValue) {
    const QString value = runValue.trimmed();
    if (value.isEmpty()) {
        return {};
    }
    if (value.startsWith(QLatin1Char('"'))) {
        const int close = static_cast<int>(value.indexOf(QLatin1Char('"'), 1));
        return close > 0 ? value.mid(1, close - 1) : value.mid(1);
    }
    const int space = static_cast<int>(value.indexOf(QLatin1Char(' ')));
    return space > 0 ? value.left(space) : value;
}

ApprovedState decodeStartupApproved(const QByteArray& blob) {
    if (blob.isEmpty()) {
        return ApprovedState::NoData;
    }
    return (static_cast<unsigned char>(blob.at(0)) & 0x01U) == 0U ? ApprovedState::Enabled
                                                                  : ApprovedState::Disabled;
}

} // namespace autostart
