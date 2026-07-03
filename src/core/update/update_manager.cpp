// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/update_manager.h"

#include "daemon_app_version.h"

#include <QMetaEnum>

// Package-time defines (see src/core/update/CMakeLists.txt). The capability
// dial arrives as a bare enumerator name (None/Notify/DownloadAndOpen/
// SelfApply) so an out-of-vocabulary value is a compile error, not a silent
// fallback; the feed URL arrives as a quoted string literal.
#ifndef DAEMON_APP_UPDATE_CAPABILITY
#define DAEMON_APP_UPDATE_CAPABILITY None
#endif
#ifndef DAEMON_APP_UPDATE_FEED_URL
#define DAEMON_APP_UPDATE_FEED_URL ""
#endif

namespace update {

UpdateManager::UpdateManager(QObject* parent)
    : QObject(parent), m_capability(Capability::DAEMON_APP_UPDATE_CAPABILITY) {}

QString UpdateManager::currentVersion() const {
    return QStringLiteral(DAEMON_APP_VERSION_STR);
}

QUrl UpdateManager::feedUrl() const {
    return {QStringLiteral(DAEMON_APP_UPDATE_FEED_URL)};
}

QString UpdateManager::capabilityToString(Capability capability) {
    return QString::fromLatin1(
        QMetaEnum::fromType<Capability>().valueToKey(static_cast<int>(capability)));
}

UpdateManager::Capability UpdateManager::capabilityFromString(const QString& name) {
    bool ok = false;
    const int value = QMetaEnum::fromType<Capability>().keyToValue(name.toLatin1(), &ok);
    // Fail closed: anything unrecognized is the inert dial, never an escalation.
    return ok ? static_cast<Capability>(value) : Capability::None;
}

void UpdateManager::check() {
    failNotImplemented(QStringLiteral("check"));
}

void UpdateManager::download() {
    failNotImplemented(QStringLiteral("download"));
}

void UpdateManager::apply() {
    failNotImplemented(QStringLiteral("apply"));
}

void UpdateManager::failNotImplemented(const QString& operation) {
    m_lastError = QStringLiteral("UpdateManager::%1() is not implemented — updates are "
                                 "design/scaffolding only (packaging/UPDATES.md)")
                      .arg(operation);
    if (m_state != State::Error) {
        m_state = State::Error;
        emit stateChanged();
    }
    emit errorOccurred(m_lastError);
}

} // namespace update
