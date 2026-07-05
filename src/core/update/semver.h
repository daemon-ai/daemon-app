// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>

namespace update::semver {

// Strip the build-metadata suffix ("+<anything>") and any leading 'v'. The
// running build's currentVersion carries a git suffix (X.Y.Z+<n>.g<hash>[.dirty])
// that must be dropped before a precedence comparison (SemVer 2.0.0 §10: build
// metadata is ignored in precedence).
[[nodiscard]] QString stripBuildMetadata(const QString& version);

// SemVer 2.0.0 precedence compare: returns -1 if a < b, 0 if equal, 1 if a > b.
// Build metadata is ignored; a prerelease version ranks below its release core
// (1.0.0-rc.1 < 1.0.0); prerelease identifiers compare left-to-right (numeric
// numerically and below alphanumeric, alphanumeric ASCII-lexically, a longer set
// of identifiers outranking a shorter prefix). Non-numeric core components fail
// closed to 0 for that component (a malformed feed version never registers as an
// upgrade on its own).
[[nodiscard]] int compare(const QString& a, const QString& b);

// True iff `candidate` is a strict upgrade over `current` (candidate > current
// after both are stripped of build metadata). The downgrade-replay guard.
[[nodiscard]] bool isUpgrade(const QString& candidate, const QString& current);

} // namespace update::semver
