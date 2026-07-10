// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "local_database.h"

// RED stub: real implementation lands in the GREEN commit.
namespace mirror {

LocalDatabase::LocalDatabase(const QString&, QObject* parent) : QObject(parent) {}
LocalDatabase::~LocalDatabase() = default;

void LocalDatabase::setIdentityNamespace(const QString&) {}
bool LocalDatabase::isOpen() const {
    return false;
}
int LocalDatabase::schemaVersion() const {
    return 0;
}
bool LocalDatabase::insertOp(const PendingOp&) {
    return false;
}
bool LocalDatabase::updateOp(const PendingOp&) {
    return false;
}
bool LocalDatabase::deleteOp(const QString&) {
    return false;
}
QList<PendingOp> LocalDatabase::loadOps() const {
    return {};
}
int LocalDatabase::laneDepth(const QString&) const {
    return 0;
}
int LocalDatabase::totalDepth() const {
    return 0;
}
bool LocalDatabase::resetInflightToPending() {
    return false;
}
bool LocalDatabase::upsertSidecarConversation(const SidecarConversation&) {
    return false;
}
SidecarConversation LocalDatabase::sidecarConversation(const QString&) const {
    return {};
}
QList<SidecarConversation> LocalDatabase::sidecarConversations() const {
    return {};
}
bool LocalDatabase::upsertSidecarSession(const SidecarSession&) {
    return false;
}
SidecarSession LocalDatabase::sidecarSession(const QString&) const {
    return {};
}
bool LocalDatabase::setMeta(const QString&, const QString&) {
    return false;
}
QString LocalDatabase::meta(const QString&) const {
    return {};
}
bool LocalDatabase::hasColumn(const QString&, const QString&) const {
    return false;
}

QString LocalDatabase::defaultDatabasePath() {
    return {};
}
QString LocalDatabase::namespacedPath(const QString&) const {
    return {};
}
void LocalDatabase::openAt(const QString&) {}
bool LocalDatabase::openConnection() {
    return false;
}
void LocalDatabase::dropConnection() {}
bool LocalDatabase::ensureSchema() {
    return false;
}
bool LocalDatabase::createCurrentTables() {
    return false;
}
bool LocalDatabase::migrateFrom(int) {
    return false;
}
bool LocalDatabase::execSql(const char*) {
    return false;
}
void LocalDatabase::setLastError(const QString&) const {}

} // namespace mirror
