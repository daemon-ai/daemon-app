// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "verb_class.h"

// RED stub: real implementation lands in the GREEN commit.
namespace mirror {

VerbClass verbClass(const QString&) {
    return VerbClass::Unknown;
}

LaneKind laneKind(VerbClass) {
    return LaneKind::Mutation;
}

bool isCoalescing(VerbClass) {
    return false;
}

bool requiresConnected(VerbClass) {
    return true;
}

QString laneClassName(VerbClass) {
    return {};
}

QString buildLane(VerbClass, const QString&) {
    return {};
}

ErrorClass classifyError(const QString&) {
    return ErrorClass::Rejection;
}

} // namespace mirror
