#include "fs/ifs_service.h"

namespace fs {

void registerFsMetatypes()
{
    qRegisterMetaType<fs::FsEntry>("fs::FsEntry");
    qRegisterMetaType<fs::FsRoot>("fs::FsRoot");
    qRegisterMetaType<fs::FsSearchHit>("fs::FsSearchHit");
    qRegisterMetaType<QList<fs::FsEntry>>("QList<fs::FsEntry>");
    qRegisterMetaType<QList<fs::FsRoot>>("QList<fs::FsRoot>");
    qRegisterMetaType<QList<fs::FsSearchHit>>("QList<fs::FsSearchHit>");
}

} // namespace fs
