// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "transports/mock_persons_service.h"

namespace transports {

namespace {

QVariantMap endpoint(const QString& transport, const QString& id, const QString& name) {
    QVariantMap e;
    e[QStringLiteral("transport")] = transport;
    e[QStringLiteral("id")] = id;
    e[QStringLiteral("displayName")] = name;
    e[QStringLiteral("presence")] = QStringLiteral("available");
    e[QStringLiteral("permission")] = QString();
    return e;
}

} // namespace

MockPersonsService::MockPersonsService(QObject* parent) : IPersonsService(parent) {
    // A canned person consistent with MockTransportRegistry's demo Matrix account, so the Persons
    // section renders offline (UI-first).
    QVariantMap p;
    p[QStringLiteral("id")] = QStringLiteral("person-bob");
    p[QStringLiteral("alias")] = QStringLiteral("Bob");
    p[QStringLiteral("endpoints")] =
        QVariantList{endpoint(QStringLiteral("matrix/@bot:hs"), QStringLiteral("@bob:matrix.org"),
                              QStringLiteral("Bob"))};
    m_persons.append(p);
}

QVariantList MockPersonsService::persons() const {
    return m_persons;
}

void MockPersonsService::refresh() {
    emit personsChanged(m_persons);
}

} // namespace transports
