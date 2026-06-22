#pragma once

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>

namespace domain {

// A chat thread. For now the whole transcript is a single markdown string
// (`content`); a structured message model can replace it later behind the store
// seam without changing this type's role.
struct Conversation {
    int id = -1;
    // The agent/unit this conversation belongs to (an AgentNode id; empty if
    // unassigned). Maps later to a unit's transcript drained via
    // `unit_outbound(unitId)`: a leaf engine owns exactly one conversation,
    // while a parent node folds its whole subtree's conversations.
    QString agentId;
    QList<int> tagIds;
    QString title;
    QString content; // markdown
    bool isArchived = false;
    // Pinned conversations float to the top of any list scope (client-side
    // session action; the daemon will later carry the same flag).
    bool isPinned = false;
    QDateTime created;
    QDateTime modified;

    [[nodiscard]] bool isValid() const { return id >= 0; }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::Conversation)
