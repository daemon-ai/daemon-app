// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QDir>
#include <QStandardPaths>

// The daemon-config-domain mocks (profiles / accounts / models / cron / daemon
// config) now keep a last-known JSON cache under AppDataLocation, loading it on
// construct. Tests must therefore (a) redirect that location into Qt's isolated
// test sandbox so they never read or clobber the real user dir, and (b) wipe it
// before every case so each construction starts from the seed instead of a prior
// case's persisted mutation. Call from an `init()` slot (runs before each test).
inline void resetMockCache() {
    QStandardPaths::setTestModeEnabled(true);
    QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively();
}
