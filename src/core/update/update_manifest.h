// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace update {

// One downloadable artifact row (packaging/UPDATES.md schema 1, per-artifact).
struct Artifact {
    QString kind;             // appimage|deb|rpm|nsis|dmg|portable-tar|apk
    QString os;               // linux|windows|macos|android
    QString arch;             // x86_64|aarch64|unknown
    QString file;             // resolved relative to the manifest URL
    qint64 size = 0;          // exact byte count (cheap pre-gate)
    QString sha256;           // hex digest; the full-download gate
    QString glibcFloor;       // linux only, optional: min glibc the binary needs
    QString updateCapability; // the feed dial: None|Notify|DownloadAndOpen|SelfApply
    QString zsync;            // optional delta control file (transport not impl.)
};

// A parsed schema-1 manifest (packaging/UPDATES.md top level).
struct Manifest {
    int schema = 0;
    QString product;
    QString channel;
    QString version;
    QString released;
    QString notesUrl;
    QList<Artifact> artifacts;
};

struct ManifestParseResult {
    bool ok = false;
    QString error; // empty on success
    Manifest manifest;
};

// Parse + validate strict schema 1. Rejects: non-object JSON, an unknown major
// schema, a product mismatch, and a channel mismatch (each is a fail-closed
// discard - a manifest served for the wrong product/channel is never parsed
// further). `expectedProduct` is always "daemon"; `expectedChannel` is the
// client's compiled channel.
[[nodiscard]] ManifestParseResult parseManifest(const QByteArray& json,
                                                const QString& expectedProduct,
                                                const QString& expectedChannel);

// The host's glibc version string (e.g. "2.38"), or empty when it cannot be
// determined (non-glibc platform, or the query is unavailable). Used to enforce
// an artifact's glibcFloor on Linux.
[[nodiscard]] QString hostGlibcVersion();

// The criteria this build selects an artifact by: its compiled artifact kind, os,
// arch, plus the host glibc for the Linux floor check (empty elsewhere).
struct SelectionCriteria {
    QString kind;
    QString os;
    QString arch;
    QString hostGlibc; // empty = skip the floor check
};

struct SelectionResult {
    bool matched = false;
    Artifact artifact;
    QString reason; // why no row matched (empty on success)
};

// Pick the single artifact row this build can install: os + arch + kind must
// match, arch=unknown is never selected, and on Linux an artifact whose
// glibcFloor exceeds the host glibc is skipped. Returns the first match.
[[nodiscard]] SelectionResult selectArtifact(const Manifest& manifest,
                                             const SelectionCriteria& criteria);

} // namespace update
