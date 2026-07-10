// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QVariantList>

namespace transports {

// [integrations wire v38] Persons / metacontacts seam: the client-side view of the node's person
// registry (PersonList) — the cross-transport identities the integrations tree renders under each
// account's "Persons" section. A person groups the contact endpoints reachable across transports
// (person-endpoint: transport + contact). Read-only at v38: the node owns the registry and re-emits
// on PersonsChanged; the app renders, never derives. A daemon adapter (DaemonPersonsService)
// decodes the node's PersonList + refetches on the PersonsChanged feed event; the mock returns
// canned rows so the surface can be built + exercised offline (UI-first).
//
// Sibling of ITransportRegistry / IContactsService (kept cohesive — persons are NOT the per-account
// roster): a person may span multiple accounts, so it is its own seam.
class IPersonsService : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IPersonsService() override = default;

    // The last-known persons (PersonList). Each entry is a display-only map:
    //   id (QString), alias (QString; empty = none),
    //   endpoints (QVariantList of { transport, id, displayName, presence } maps).
    // `persons()` returns the cached set; `refresh()` triggers a live enumeration and fires
    // personsChanged when it lands. Default returns empty so non-daemon seams need not implement.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList persons() const { return {}; }
    Q_INVOKABLE virtual void refresh() {}

signals:
    // The person registry changed (a refresh landed / the node's PersonsChanged feed event fired).
    // `persons` is the fresh row list (same shape as persons()).
    void personsChanged(const QVariantList& persons);
    // A PersonList/registry op failed on the node (surfaced as a toast / TUI notice).
    void personsOperationFailed(const QString& message);
};

} // namespace transports
