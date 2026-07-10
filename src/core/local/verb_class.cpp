// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "verb_class.h"

#include <QChar>

namespace mirror {
namespace {
// Canonical lane separator: the unit separator, matching the repositories' composite-key
// convention (spec 09 §3.1) and the entity-map key serialization.
constexpr char16_t kSep = u'\x1f';
} // namespace

VerbClass verbClass(const QString& verb) {
    if (verb == QLatin1String("ConvSend")) {
        return VerbClass::ChatSend;
    }
    if (verb == QLatin1String("ConvSetTopic") || verb == QLatin1String("ConvSetTitle") ||
        verb == QLatin1String("ConvSetDescription")) {
        return VerbClass::ConvMeta;
    }
    if (verb == QLatin1String("RosterAdd") || verb == QLatin1String("RosterUpdate") ||
        verb == QLatin1String("RosterRemove") || verb == QLatin1String("ContactSetAlias")) {
        return VerbClass::RosterEdit;
    }
    if (verb == QLatin1String("SessionUpdateMeta")) {
        return VerbClass::SessionMeta;
    }
    if (verb == QLatin1String("TurnPrompt")) {
        return VerbClass::TurnPrompt;
    }
    return VerbClass::Unknown;
}

LaneKind laneKind(VerbClass cls) {
    // The turn-prompt lane is a dispatch lane (consumed at drain into Submit); every wire-verb
    // class is a mutation lane (confirmed by a provenance-stamped read).
    return cls == VerbClass::TurnPrompt ? LaneKind::Dispatch : LaneKind::Mutation;
}

bool isCoalescing(VerbClass cls) {
    // Latest-wins per (verb, target): only conv-meta (§6.4).
    return cls == VerbClass::ConvMeta;
}

bool requiresConnected(VerbClass cls) {
    // Every mutation (wire) lane needs a connected+authenticated wire to drain; the turn-prompt
    // dispatch lane is gated on session-idle instead, handled by the drainer's predicate.
    return laneKind(cls) == LaneKind::Mutation;
}

QString laneClassName(VerbClass cls) {
    switch (cls) {
    case VerbClass::ChatSend:
        return QStringLiteral("chat-send");
    case VerbClass::ConvMeta:
        return QStringLiteral("conv-meta");
    case VerbClass::RosterEdit:
        return QStringLiteral("roster-edit");
    case VerbClass::SessionMeta:
        return QStringLiteral("session-meta");
    case VerbClass::TurnPrompt:
        return QStringLiteral("turn-prompt");
    case VerbClass::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString buildLane(VerbClass cls, const QString& scope) {
    return laneClassName(cls) + QChar(kSep) + scope;
}

ErrorClass classifyError(const QString& apiErrorKind) {
    if (apiErrorKind == QLatin1String("Unauthenticated")) {
        return ErrorClass::Unauthenticated;
    }
    // Connection-level failures are transient: back off, keep order (§6.5).
    if (apiErrorKind == QLatin1String("Timeout") || apiErrorKind == QLatin1String("Disconnected") ||
        apiErrorKind == QLatin1String("ConnectionLost") ||
        apiErrorKind == QLatin1String("Unavailable")) {
        return ErrorClass::Transient;
    }
    // Unsupported / Conflict / Forbidden / UnknownSession / Other — and ANYTHING unknown — are
    // terminal rejections (fail visible; §6.5 "anything new defaults to rejection").
    return ErrorClass::Rejection;
}

} // namespace mirror
