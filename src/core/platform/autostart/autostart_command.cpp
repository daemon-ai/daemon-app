// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/autostart/autostart_command.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace autostart {

QString hiddenArg() {
    return QStringLiteral("--hidden");
}

QString resolveProgramFromParts(const QString& appImagePath, const QString& applicationFilePath) {
    if (!appImagePath.isEmpty()) {
        return appImagePath;
    }
    return applicationFilePath;
}

QString resolveProgramForCurrentApp() {
    return resolveProgramFromParts(qEnvironmentVariable("APPIMAGE"),
                                   QCoreApplication::applicationFilePath());
}

QString resolveSiblingGuiProgramInDir(const QString& dir) {
#ifdef Q_OS_WIN
    const QString candidate = QDir(dir).filePath(QStringLiteral("daemon-app.exe"));
#else
    const QString candidate = QDir(dir).filePath(QStringLiteral("daemon-app"));
#endif
    const QFileInfo info(candidate);
    if (info.exists() && info.isFile() && info.isExecutable()) {
        return info.absoluteFilePath();
    }
    return {};
}

QString resolveSiblingGuiProgram() {
    // Inside an AppImage the mount path dies with the process; the entry must
    // point at the .AppImage itself (which launches the GUI by default).
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    if (!appImage.isEmpty()) {
        return appImage;
    }
    return resolveSiblingGuiProgramInDir(
        QFileInfo(QCoreApplication::applicationFilePath()).absolutePath());
}

bool runningInsideFlatpak() {
    return QFile::exists(QStringLiteral("/.flatpak-info"));
}

} // namespace autostart
