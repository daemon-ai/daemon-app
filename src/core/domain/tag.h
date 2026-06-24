#pragma once

#include <QMetaType>
#include <QString>

namespace domain {

// A label that can be attached to sessions.
struct Tag {
    int id = -1;
    QString name;
    QString color; // #rrggbb

    [[nodiscard]] bool isValid() const { return id >= 0; }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::Tag)
