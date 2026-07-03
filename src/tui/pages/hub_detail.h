// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "uimodels/variant_list_model.h"

#include <QList>
#include <QObject>
#include <QVariantMap>

// Internal helpers shared by the TuiPageHub dispatcher (tui_page_hub.cpp) and
// the per-page builder TUs (pages/hub_*.cpp). Not part of the hub's API.
namespace hubdetail {

inline QList<QVariantMap> rowsOfModel(QObject* m) {
    auto* vlm = qobject_cast<uimodels::VariantListModel*>(m);
    return vlm ? vlm->rows() : QList<QVariantMap>{};
}

} // namespace hubdetail
