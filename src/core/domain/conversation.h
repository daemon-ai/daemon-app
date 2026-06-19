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
    int folderId = -1;
    QList<int> tagIds;
    QString title;
    QString content; // markdown
    bool isArchived = false;
    QDateTime created;
    QDateTime modified;

    [[nodiscard]] bool isValid() const { return id >= 0; }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::Conversation)
