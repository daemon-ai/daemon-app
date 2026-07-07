// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "core/agent_block.h" // be::parseUnifiedDiff

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantList>

namespace be {

// [waveB:app-v30] D5: a controller-free QML singleton that surfaces the SAME unified-diff line
// parser (be::parseUnifiedDiff) that EditorController::parseDiff / ToolCallBlock use. Lets a
// surface with no EditorController (e.g. the Approvals card) feed a DiffBlock without duplicating
// the parse logic. Pure, stateless — no per-transcript state, so a singleton is correct.
class DiffFormat : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    using QObject::QObject;

    // Split a unified diff into typed lines ({ text, kind }); the same shape DiffBlock renders.
    [[nodiscard]] Q_INVOKABLE QVariantList parse(const QString& diff) const {
        return parseUnifiedDiff(diff);
    }
};

} // namespace be
