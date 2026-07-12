// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "domain/ids.h"
#include "domain/unit_node.h" // SessionRole

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>

namespace domain {

// The lifecycle of a session (mirrors daemon-api `Lifecycle`): a durable, persisted
// incarnation vs an ephemeral live/interactive one.
enum class Lifecycle {
    Durable,
    Live,
};

// The coarse run-state of a session (mirrors daemon-api `SessionState`; the wire
// `Suspended { job_id }` payload is flattened to the enum + `suspendedJob`).
enum class SessionState {
    Active,
    Suspended,
    Ready,
    Completed,
    Unknown,
};

// A session: a running session/transcript. This mirrors the daemon wire
// `SessionInfo` field-for-field so a mock and a future daemon adapter are a backend
// swap, not a reshape. `sessionId` (string) is the AUTHORITATIVE identity used by
// NodeApi/cache/DaemonNet; `id` (int) is a demoted, local presentation handle kept
// only while the UI/transcript pipeline finishes migrating to `sessionId`.
struct Session {
    // Authoritative wire identity (`daemon-common::SessionId`).
    SessionId sessionId;
    // Demoted local presentation handle (tab/list row); -1 = none. Do not persist or
    // send; prefer `sessionId` everywhere.
    int id = -1;

    // The unit this session belongs to (a `UnitId`; empty if unassigned).
    UnitId unitId;
    // The agent/profile this session runs under (`SessionInfo.bound_profile`).
    ProfileRef boundProfile;

    QString title;
    QString content; // markdown (legacy; transcripts migrate to SessionLogEntry)

    SessionState state = SessionState::Unknown;
    Lifecycle lifecycle = Lifecycle::Durable;
    SessionRole role = SessionRole::Primary;
    SessionId parent; // parent session (`SessionInfo.parent`), empty if top-level

    bool isArchived = false;
    // Pinned sessions float to the top of any list scope (the daemon carries the
    // same flag on `SessionInfo`).
    bool isPinned = false;
    qint64 lastActivityMs = 0; // `SessionInfo.last_activity_ms`

    QDateTime created;
    QDateTime modified;

    // Valid if it carries either identity. Sentinels (default-constructed) carry
    // neither. Tolerant during the int->SessionId migration; once the pipeline is
    // fully SessionId-keyed this becomes `!sessionId.isEmpty()`.
    [[nodiscard]] bool isValid() const { return id >= 0 || !sessionId.isEmpty(); }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::Session)
