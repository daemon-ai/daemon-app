#pragma once

#include <QMetaType>
#include <QString>

namespace domain {

// A folder / project grouping conversations. Flat for now (parentId reserved
// for nesting later).
struct Folder {
    int id = -1;
    int parentId = -1;
    QString name;

    [[nodiscard]] bool isValid() const { return id >= 0; }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::Folder)
