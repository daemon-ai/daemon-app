// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/ipersons_service.h"

namespace transports {

// [integrations wire v38] Inert dev stand-in for IPersonsService: carries a canned person registry
// (consistent with MockTransportRegistry's canned Matrix account) so the Persons surface can be
// built + exercised offline (UI-first). refresh() re-emits the canned set. Replaced by
// DaemonPersonsService (decoding the node's PersonList) whenever a daemon connects.
class MockPersonsService : public IPersonsService {
    Q_OBJECT

public:
    explicit MockPersonsService(QObject* parent = nullptr);

    [[nodiscard]] QVariantList persons() const override;
    void refresh() override;

private:
    QVariantList m_persons;
};

} // namespace transports
