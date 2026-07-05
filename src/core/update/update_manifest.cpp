// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/update_manifest.h"

#include "update/semver.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#if defined(__linux__) && defined(__GLIBC__)
#include <gnu/libc-version.h>
#endif

namespace update {

namespace {

ManifestParseResult parseFail(const QString& reason) {
    return {false, reason, Manifest{}};
}

Artifact readArtifact(const QJsonObject& obj) {
    Artifact a;
    a.kind = obj.value(QStringLiteral("kind")).toString();
    a.os = obj.value(QStringLiteral("os")).toString();
    a.arch = obj.value(QStringLiteral("arch")).toString();
    a.file = obj.value(QStringLiteral("file")).toString();
    // size is an int64 byte count; QJsonValue stores numbers as double, so read
    // via toVariant to preserve the full range for large artifacts.
    a.size = obj.value(QStringLiteral("size")).toVariant().toLongLong();
    a.sha256 = obj.value(QStringLiteral("sha256")).toString();
    a.glibcFloor = obj.value(QStringLiteral("glibcFloor")).toString();
    a.updateCapability = obj.value(QStringLiteral("updateCapability")).toString();
    a.zsync = obj.value(QStringLiteral("zsync")).toString();
    return a;
}

} // namespace

ManifestParseResult parseManifest(const QByteArray& json, const QString& expectedProduct,
                                  const QString& expectedChannel) {
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return parseFail(QStringLiteral("manifest is not valid JSON"));
    }
    const QJsonObject root = doc.object();

    Manifest m;
    m.schema = root.value(QStringLiteral("schema")).toInt(0);
    if (m.schema != 1) {
        // Reject unknown majors: a schema this client does not understand must be
        // discarded, not best-effort parsed.
        return parseFail(QStringLiteral("unsupported manifest schema %1").arg(m.schema));
    }

    m.product = root.value(QStringLiteral("product")).toString();
    if (m.product != expectedProduct) {
        return parseFail(QStringLiteral("manifest product '%1' does not match '%2'")
                             .arg(m.product, expectedProduct));
    }

    m.channel = root.value(QStringLiteral("channel")).toString();
    if (m.channel != expectedChannel) {
        return parseFail(QStringLiteral("manifest channel '%1' does not match '%2'")
                             .arg(m.channel, expectedChannel));
    }

    m.version = root.value(QStringLiteral("version")).toString();
    if (m.version.isEmpty()) {
        return parseFail(QStringLiteral("manifest has no version"));
    }
    m.released = root.value(QStringLiteral("released")).toString();
    m.notesUrl = root.value(QStringLiteral("notesUrl")).toString();

    const QJsonValue artifacts = root.value(QStringLiteral("artifacts"));
    if (!artifacts.isArray()) {
        return parseFail(QStringLiteral("manifest artifacts is not an array"));
    }
    const QJsonArray arr = artifacts.toArray();
    m.artifacts.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        if (v.isObject()) {
            m.artifacts.append(readArtifact(v.toObject()));
        }
    }

    return {true, QString(), m};
}

QString hostGlibcVersion() {
#if defined(__linux__) && defined(__GLIBC__)
    return QString::fromLatin1(gnu_get_libc_version());
#else
    return {};
#endif
}

SelectionResult selectArtifact(const Manifest& manifest, const SelectionCriteria& criteria) {
    bool sawOsArch = false;
    for (const Artifact& a : manifest.artifacts) {
        if (a.arch == QStringLiteral("unknown")) {
            continue; // never auto-select an artifact whose arch could not be parsed
        }
        if (a.os != criteria.os || a.arch != criteria.arch) {
            continue;
        }
        sawOsArch = true;
        if (a.kind != criteria.kind) {
            continue;
        }
        // Linux glibc floor: skip an artifact the host is too old to run. Only
        // enforced when both the floor and the host version are known.
        if (!a.glibcFloor.isEmpty() && !criteria.hostGlibc.isEmpty() &&
            semver::compare(criteria.hostGlibc, a.glibcFloor) < 0) {
            continue;
        }
        return {true, a, QString()};
    }

    const QString reason =
        sawOsArch ? QStringLiteral("no artifact matches kind '%1' for this host").arg(criteria.kind)
                  : QStringLiteral("no artifact for %1/%2").arg(criteria.os, criteria.arch);
    return {false, Artifact{}, reason};
}

} // namespace update
