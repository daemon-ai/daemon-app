#pragma once

#include <QDir>
#include <QString>
#include <QUrl>

namespace be {

// True when the raw image URL points at a remote http(s) resource that must be
// downloaded (and is therefore routed through the shared CachedImageProvider).
inline bool isRemoteImage(const QString& rawUrl) {
    const QString trimmed = rawUrl.trimmed();
    return trimmed.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
           trimmed.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);
}

// Map a raw Markdown image URL to a QML-ready source string.
//
// Remote http(s) URLs are routed through the shared image://imgcache provider so
// they are downloaded and cached once. Local sources (file:/data:/qrc: and bare
// absolute paths) are returned as a directly-loadable URL and bypass the cache.
// Bare relative paths are a known gap (no document base directory exists yet) and
// are returned unchanged so they at least resolve against the working directory.
inline QString resolveImageSource(const QString& rawUrl) {
    const QString trimmed = rawUrl.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    if (isRemoteImage(trimmed)) {
        return QStringLiteral("image://imgcache/") +
               QString::fromLatin1(QUrl::toPercentEncoding(trimmed));
    }

    // Already a usable local/embedded URL scheme: load directly, no caching.
    if (trimmed.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("data:"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("qrc:"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("image://"), Qt::CaseInsensitive)) {
        return trimmed;
    }

    // A bare ":/..." Qt resource path.
    if (trimmed.startsWith(QLatin1Char(':'))) {
        return QStringLiteral("qrc") + trimmed;
    }

    // A bare absolute filesystem path -> file:// URL (loaded directly).
    if (QDir::isAbsolutePath(trimmed)) {
        return QUrl::fromLocalFile(trimmed).toString();
    }

    // Relative path: no document base directory to resolve against (see plan).
    return trimmed;
}

} // namespace be
