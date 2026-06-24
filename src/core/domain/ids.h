#pragma once

#include <QHash>
#include <QMetaType>
#include <QString>

#include <utility>

// Typed identifier newtypes mirroring the daemon `string_id!` newtypes
// (`daemon-common`). They give the frontend the same vocabulary and a measure of
// type-safety in the C++ core/seam/domain layer. The QML boundary stays QString:
// model roles return `id.toString()` and Q_INVOKABLE params take QString and wrap,
// since QML cannot use these custom types ergonomically.
//
// Canonical mapping (daemon is authoritative; there is no `Agent` type):
//   - ProfileRef = the agent IDENTITY (the configured profile / ProfileSpec).
//   - UnitId     = a supervision-tree node (UnitNode; kind Engine/Host/Orchestrator).
//   - SessionId  = a running conversation incarnation backing a unit.
namespace domain {

#define DAEMON_APP_STRING_ID(Name)                                                                 \
    struct Name {                                                                                  \
        QString value;                                                                             \
        Name() = default;                                                                          \
        explicit Name(QString v) : value(std::move(v)) {}                                          \
        [[nodiscard]] bool isEmpty() const { return value.isEmpty(); }                             \
        [[nodiscard]] QString toString() const { return value; }                                  \
        friend bool operator==(const Name& a, const Name& b) { return a.value == b.value; }        \
        friend bool operator!=(const Name& a, const Name& b) { return a.value != b.value; }        \
    };                                                                                             \
    inline size_t qHash(const Name& id, size_t seed = 0) noexcept                                  \
    {                                                                                              \
        return qHash(id.value, seed);                                                              \
    }

// A reference to a provider profile == the agent identity (`ProfileSpec.id`).
DAEMON_APP_STRING_ID(ProfileRef)
// A managed unit in the supervision tree (`daemon-protocol::UnitId`).
DAEMON_APP_STRING_ID(UnitId)
// A durable engine incarnation / running conversation (`daemon-common::SessionId`).
DAEMON_APP_STRING_ID(SessionId)

#undef DAEMON_APP_STRING_ID

} // namespace domain

Q_DECLARE_METATYPE(domain::ProfileRef)
Q_DECLARE_METATYPE(domain::UnitId)
Q_DECLARE_METATYPE(domain::SessionId)
