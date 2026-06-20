#pragma once

#include <QByteArray>
#include <QColor>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace be {

// The decoded payload of an image://math/<id> request: the LaTeX source plus the
// render parameters baked into the URL so a theme/size change yields a different
// id (and therefore a fresh, correctly-colored raster).
struct MathRequest {
    QString latex;
    bool displayMode = false;
    int fontPx = 15;
    quint32 colorArgb = 0xff000000;
    bool valid = false;
};

// Build the image://math/<payload> source consumed by MathImageProvider. The
// LaTeX is base64url-encoded so it can carry any character (including '&', '=',
// '%', '/') without colliding with the field separators or the provider's URL
// parsing, regardless of how Qt percent-handles the image id.
inline QString mathImageUrl(const QString &latex, bool displayMode, int fontPx, const QColor &color)
{
    const QColor resolved = color.isValid() ? color : QColor(Qt::black);
    const QByteArray latexB64 =
        latex.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    const QString payload = QStringLiteral("d=%1&s=%2&c=%3&l=%4")
                                .arg(displayMode ? 1 : 0)
                                .arg(fontPx)
                                .arg(static_cast<quint32>(resolved.rgba()), 8, 16, QLatin1Char('0'))
                                .arg(QString::fromLatin1(latexB64));
    return QStringLiteral("image://math/") + payload;
}

// Parse the <payload> id passed to MathImageProvider::requestImage back into a
// MathRequest. Returns a request with valid==false when the LaTeX is empty.
inline MathRequest parseMathImageId(const QString &id)
{
    MathRequest req;
    const QStringList parts = id.split(QLatin1Char('&'), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const qsizetype eq = part.indexOf(QLatin1Char('='));
        if (eq < 0) {
            continue;
        }
        const QString key = part.left(eq);
        const QString value = part.mid(eq + 1);
        if (key == QLatin1String("d")) {
            req.displayMode = value == QLatin1String("1");
        } else if (key == QLatin1String("s")) {
            bool ok = false;
            const int px = value.toInt(&ok);
            if (ok && px > 0) {
                req.fontPx = px;
            }
        } else if (key == QLatin1String("c")) {
            bool ok = false;
            const quint32 argb = value.toUInt(&ok, 16);
            if (ok) {
                req.colorArgb = argb;
            }
        } else if (key == QLatin1String("l")) {
            req.latex = QString::fromUtf8(
                QByteArray::fromBase64(value.toLatin1(), QByteArray::Base64UrlEncoding));
        }
    }
    req.valid = !req.latex.isEmpty();
    return req;
}

// Detect block-level math directly from a block's raw markdown and return the
// bare LaTeX, or empty when the block is not math. Two forms are recognized:
//   * the whole block is a single $$ ... $$ display span, or
//   * a fenced ```math / ~~~math block (fence lines stripped).
// Inline `$...$` within prose is intentionally NOT matched here (it stays a
// paragraph and renders through the inline projector).
inline QString blockMathSource(const QString &markdown)
{
    static const QRegularExpression displayRe(
        QStringLiteral("\\A\\s*\\$\\$([\\s\\S]+?)\\$\\$\\s*\\z"));
    const QRegularExpressionMatch dm = displayRe.match(markdown);
    if (dm.hasMatch()) {
        return dm.captured(1).trimmed();
    }

    QStringList lines = markdown.split(QLatin1Char('\n'));
    if (lines.isEmpty()) {
        return QString();
    }
    const QString first = lines.first().trimmed();
    if (!first.startsWith(QStringLiteral("```")) && !first.startsWith(QStringLiteral("~~~"))) {
        return QString();
    }
    QString lang = first.mid(3).trimmed();
    const qsizetype sp = lang.indexOf(QLatin1Char(' '));
    if (sp >= 0) {
        lang = lang.left(sp);
    }
    if (lang.compare(QStringLiteral("math"), Qt::CaseInsensitive) != 0) {
        return QString();
    }
    lines.removeFirst();
    if (!lines.isEmpty()) {
        const QString last = lines.last().trimmed();
        if (last.startsWith(QStringLiteral("```")) || last.startsWith(QStringLiteral("~~~"))) {
            lines.removeLast();
        }
    }
    return lines.join(QLatin1Char('\n')).trimmed();
}

} // namespace be
