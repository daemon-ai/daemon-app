// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QMetaType>
#include <QString>

namespace fs {

// One directory child as served by IFsService. Paths are root-relative with
// POSIX separators ("" is the root itself). `revisionHint` is opaque (the dev
// impl uses mtime+size); consumers must not parse it.
struct FsEntry {
    QString name;
    QString path; // root-relative, POSIX separators
    bool isDir = false;
    bool symlink = false;
    bool ignored = false; // matched an ignore rule (dim, don't hide)
    qint64 size = 0;
    qint64 mtimeMs = 0;
};

// An opened root on the connected node. `id` is opaque; `unitId` is non-empty
// when this root is a unit's ExecutionEnvironment sandbox (always empty in the
// dev impl). The GUI never sees absolute host paths.
struct FsRoot {
    QString id;
    QString label;
    QString unitId;
};

// One project-search hit (server-side search in production; recursive walk in
// the dev impl). `line`/`column` are 1-based.
struct FsSearchHit {
    QString path;
    int line = 1;
    int column = 1;
    QString preview; // the matching line, trimmed for display
};

} // namespace fs

Q_DECLARE_METATYPE(fs::FsEntry)
Q_DECLARE_METATYPE(fs::FsRoot)
Q_DECLARE_METATYPE(fs::FsSearchHit)
