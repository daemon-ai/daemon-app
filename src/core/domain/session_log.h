#pragma once

#include "domain/origin.h"

#include <QMetaType>
#include <QString>
#include <QVariantMap>

namespace domain {

// A merged-log entry direction — mirrors daemon-protocol `Direction`.
enum class Direction {
    Inbound,
    Outbound,
};

// Whether an entry enters the conversation/prompt or is observability-only —
// mirrors daemon-protocol `Disposition`.
enum class Disposition {
    Context,
    Transport,
};

// A client view of daemon-protocol `SessionLogEntry`. The wire `payload` is a rich
// union (`AgentEvent`/`HostRequest`/`AgentCommand`/`HostResponse`/`Meta`); the client
// keeps the envelope (`seq`/`direction`/`origin`/`disposition`) plus the **decoded
// payload** as a tagged `QVariantMap` (not a flattened string) so the transcript can
// reconstruct rich blocks without loss. `seq` is the monotonic cursor used to resume a
// `Subscribe`.
//
// `kind` discriminates the payload shape the renderer routes on:
//   "message"   -> a role boundary; payload { role: "user"|"assistant"|"system", id }
//   "reasoning" -> payload is the reasoning block metadata (status/durationMs/body)
//   "tool"      -> payload is the tool block metadata (callId/name/status/detail...)
//   "content"   -> payload { kind, body } (a typed content stream)
//   "text"      -> payload { markdown } (prose: paragraphs/headings/lists/code)
// For the mock these are decomposed from a session's markdown; a daemon adapter fills
// the same shapes from the decoded wire `SessionPayload`.
struct SessionLogEntry {
    quint64 seq = 0;
    Direction direction = Direction::Outbound;
    Origin origin;
    Disposition disposition = Disposition::Context;
    QString kind;          // payload discriminator (see above)
    QVariantMap payload;   // the decoded, structured payload for `kind`
};

} // namespace domain

Q_DECLARE_METATYPE(domain::SessionLogEntry)
