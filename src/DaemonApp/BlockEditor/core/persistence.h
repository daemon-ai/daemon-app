#pragma once

#include "core/block_record.h"

#include <QSqlDatabase>
#include <QString>
#include <QVector>

namespace be {

class ChangedBlockStore {
public:
    bool open(const QString& path);
    bool isOpen() const;
    bool initialize();
    bool saveBlocks(const QVector<BlockRecord>& blocks);
    QVector<BlockRecord> loadBlocks() const;
    QString lastError() const;

private:
    QString m_connectionName;
    mutable QString m_lastError;
};

} // namespace be
