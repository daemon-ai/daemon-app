#pragma once

#include <QMetaType>
#include <QString>

namespace domain {

// A participant in a chat/transcript - the client-side mirror of the daemon's `ConversationMember`
// (built on `ContactInfo { id, display_name, presence }`). `isAgent` distinguishes a local agent
// participant (the daemon `Participant::Agent`) from a human contact (`Participant::Contact`).
// `presence` is the libpurple PresencePrimitive ("available" renders a green dot); `color` is the
// derived dot color (#rrggbb) so both the GUI and TUI render the indicator identically.
struct Participant {
    QString id;           // contact/member id (e.g. "agent", "@user:hs.org")
    QString name;         // display name ("Agent" / "User")
    QString presence;     // PresencePrimitive; "available" -> green
    QString color;        // #rrggbb dot color derived from presence
    bool isAgent = false; // daemon Participant::Agent vs Participant::Contact

    [[nodiscard]] bool isValid() const { return !id.isEmpty(); }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::Participant)
