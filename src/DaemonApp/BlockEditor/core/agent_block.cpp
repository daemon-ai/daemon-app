#include "core/agent_block.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

namespace be {

namespace {

// Specialized tool variant for the QML dispatch (mirrors Hermes'
// ChainToolFallback tool-name routing): clarify -> interactive Q&A,
// image_generate -> generated-image frame, everything else generic.
QString variantForName(const QString &name)
{
    const QString key = name.trimmed().toLower();
    if (key == QStringLiteral("clarify")) {
        return QStringLiteral("clarify");
    }
    if (key == QStringLiteral("image_generate")) {
        return QStringLiteral("image-generate");
    }
    return QStringLiteral("generic");
}

// A tone may be omitted on the wire; derive a sensible default from the tool
// name so the header glyph matches the specialized variant.
QString toneToKey(const QString &tone, const QString &name)
{
    if (!tone.isEmpty()) {
        return tone;
    }
    const QString key = name.trimmed().toLower();
    if (key == QStringLiteral("clarify")) {
        return QStringLiteral("agent");
    }
    if (key == QStringLiteral("image_generate")) {
        return QStringLiteral("image");
    }
    return QStringLiteral("tool");
}

QString humanDuration(qint64 ms)
{
    if (ms <= 0) {
        return {};
    }
    if (ms < 1000) {
        return QStringLiteral("%1ms").arg(ms);
    }
    const double seconds = ms / 1000.0;
    if (seconds < 10.0) {
        return QStringLiteral("%1s").arg(seconds, 0, 'f', 1);
    }
    if (seconds < 60.0) {
        return QStringLiteral("%1s").arg(qRound(seconds));
    }
    const qint64 mins = ms / 60000;
    const qint64 rem = (ms % 60000) / 1000;
    return QStringLiteral("%1m %2s").arg(mins).arg(rem);
}

// Normalize a status string to one of running/ok/error. Treats finished/done/
// complete/success as ok, and failed/error as error.
QString normalizeStatus(const QString &raw)
{
    const QString s = raw.trimmed().toLower();
    if (s == QStringLiteral("running") || s == QStringLiteral("pending") || s == QStringLiteral("started")) {
        return QStringLiteral("running");
    }
    if (s == QStringLiteral("error") || s == QStringLiteral("failed") || s == QStringLiteral("failure")) {
        return QStringLiteral("error");
    }
    return QStringLiteral("ok");
}

} // namespace

QString agentFenceInfo(BlockType type)
{
    switch (type) {
    case BlockType::Reasoning:
        return QStringLiteral("reasoning");
    case BlockType::ToolCall:
        return QStringLiteral("tool");
    case BlockType::Content:
        return QStringLiteral("content");
    default:
        return {};
    }
}

QString messageMarkerFenceInfo()
{
    return QStringLiteral("msg");
}

bool isMessageMarkerFence(const QString &info)
{
    return info.trimmed().toLower() == messageMarkerFenceInfo();
}

QByteArray serializeMessageMarker(MessageRole role, const QString &messageId)
{
    QVariantMap payload;
    payload.insert(QStringLiteral("id"), messageId);
    payload.insert(QStringLiteral("role"), messageRoleToString(role));
    // QJsonObject sorts keys, so this is a stable round-trip form.
    const QJsonObject obj = QJsonObject::fromVariantMap(payload);
    const QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    QByteArray out = "```";
    out += messageMarkerFenceInfo().toUtf8();
    out += '\n';
    out += json;
    out += "\n```";
    return out;
}

void parseMessageMarker(const QString &body, MessageRole *role, QString *messageId)
{
    const QVariantMap meta = parseAgentBlockMetadata(body);
    if (role) {
        *role = messageRoleFromString(meta.value(QStringLiteral("role")).toString());
    }
    if (messageId) {
        *messageId = meta.value(QStringLiteral("id")).toString();
    }
}

BlockType agentBlockTypeForFence(const QString &info)
{
    const QString key = info.trimmed().toLower();
    if (key == QStringLiteral("tool")) {
        return BlockType::ToolCall;
    }
    if (key == QStringLiteral("reasoning")) {
        return BlockType::Reasoning;
    }
    if (key == QStringLiteral("content")) {
        return BlockType::Content;
    }
    return BlockType::Unknown;
}

bool isAgentBlockType(BlockType type)
{
    return type == BlockType::Reasoning || type == BlockType::ToolCall || type == BlockType::Content;
}

QByteArray serializeAgentBlock(BlockType type, const QVariantMap &metadata)
{
    const QString info = agentFenceInfo(type);
    if (info.isEmpty()) {
        return {};
    }

    // QJsonObject stores keys sorted, so compact serialization is deterministic
    // (a stable round-trip). Drop the transient fenceLanguage key the parser sets
    // on every code fence so it never leaks into the payload.
    QVariantMap payload = metadata;
    payload.remove(QStringLiteral("fenceLanguage"));
    const QJsonObject obj = QJsonObject::fromVariantMap(payload);
    const QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    QByteArray out = "```";
    out += info.toUtf8();
    out += '\n';
    out += json;
    out += "\n```";
    return out;
}

QVariantMap parseAgentBlockMetadata(const QString &body)
{
    const QByteArray trimmed = body.trimmed().toUtf8();
    if (trimmed.isEmpty()) {
        return {};
    }
    QJsonParseError err {};
    const QJsonDocument doc = QJsonDocument::fromJson(trimmed, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return {};
    }
    return doc.object().toVariantMap();
}

QString fencedBodyOf(const QString &markdown)
{
    QStringList lines = markdown.split(QLatin1Char('\n'));
    if (lines.isEmpty()) {
        return {};
    }
    const QString first = lines.first().trimmed();
    if (first.startsWith(QStringLiteral("```")) || first.startsWith(QStringLiteral("~~~"))) {
        lines.removeFirst();
        if (!lines.isEmpty()) {
            const QString last = lines.last().trimmed();
            if (last.startsWith(QStringLiteral("```")) || last.startsWith(QStringLiteral("~~~"))) {
                lines.removeLast();
            }
        }
    }
    return lines.join(QLatin1Char('\n'));
}

QVariantMap buildToolView(const QVariantMap &metadata)
{
    QVariantMap view = metadata;

    const QString name = metadata.value(QStringLiteral("name")).toString();
    const QString status = normalizeStatus(metadata.value(QStringLiteral("status")).toString());
    const QString variant = variantForName(name);
    const QString tone = toneToKey(metadata.value(QStringLiteral("tone")).toString(), name);
    const QString argsSummary = metadata.value(QStringLiteral("argsSummary")).toString();

    // detailKind defaults from the variant: a finished image_generate with a
    // resolved image renders the generated-image frame; clarify routes to its
    // interactive panel. An explicit detailKind always wins.
    QString detailKind = metadata.value(QStringLiteral("detailKind")).toString();
    if (detailKind.isEmpty()) {
        if (variant == QStringLiteral("image-generate")
            && !metadata.value(QStringLiteral("imageUrl")).toString().isEmpty()) {
            detailKind = QStringLiteral("generated-image");
        }
    }

    // A dangerous tool (terminal/execute_code) pauses for approval: the bar is
    // live only while running and not yet decided. `approval` (approved/denied)
    // is set once the user answers, which clears the gate.
    const bool needsApproval = metadata.value(QStringLiteral("needsApproval")).toBool();
    const QString approval = metadata.value(QStringLiteral("approval")).toString();
    const bool awaitingApproval = needsApproval && status == QStringLiteral("running") && approval.isEmpty();

    // Title flips with status: a present-progressive verb while running, the
    // tool name once settled (mirrors Hermes ToolView's running/done title).
    QString title;
    if (variant == QStringLiteral("clarify")) {
        title = metadata.value(QStringLiteral("answered")).toBool() ? QStringLiteral("Answered")
                                                                     : QStringLiteral("Needs your input");
    } else if (status == QStringLiteral("running")) {
        title = name.isEmpty() ? QStringLiteral("Working") : QStringLiteral("Running %1").arg(name);
    } else if (status == QStringLiteral("error")) {
        title = name.isEmpty() ? QStringLiteral("Tool failed") : name;
    } else {
        title = name.isEmpty() ? QStringLiteral("Tool") : name;
    }

    view.insert(QStringLiteral("name"), name);
    view.insert(QStringLiteral("variant"), variant);
    view.insert(QStringLiteral("title"), title);
    view.insert(QStringLiteral("subtitle"), argsSummary);
    view.insert(QStringLiteral("status"), status);
    view.insert(QStringLiteral("tone"), tone);
    view.insert(QStringLiteral("durationLabel"),
                humanDuration(metadata.value(QStringLiteral("durationMs")).toLongLong()));
    view.insert(QStringLiteral("detailKind"), detailKind);
    view.insert(QStringLiteral("awaitingApproval"), awaitingApproval);
    return view;
}

QVariantMap buildReasoningView(const QVariantMap &metadata)
{
    QVariantMap view;
    const QString status = metadata.value(QStringLiteral("status")).toString().trimmed().toLower();
    view.insert(QStringLiteral("status"), status == QStringLiteral("running") ? QStringLiteral("running")
                                                                              : QStringLiteral("complete"));
    view.insert(QStringLiteral("durationLabel"),
                humanDuration(metadata.value(QStringLiteral("durationMs")).toLongLong()));
    view.insert(QStringLiteral("body"), metadata.value(QStringLiteral("body")).toString());
    return view;
}

QVariantMap buildContentView(const QVariantMap &metadata)
{
    QVariantMap view;
    view.insert(QStringLiteral("kind"), metadata.value(QStringLiteral("kind")).toString());
    view.insert(QStringLiteral("body"), metadata.value(QStringLiteral("body")).toString());
    return view;
}

QVariantMap clarifyAnswerPatch(const QVariantMap &answers)
{
    // Derive a flat human summary for the compact resolved row and for older
    // single-question consumers reading `answer` (lists are comma-joined; the
    // per-answer summaries are then joined with "; ").
    QStringList parts;
    for (auto it = answers.cbegin(); it != answers.cend(); ++it) {
        if (it.value().typeId() == QMetaType::QVariantList) {
            QStringList items;
            const QVariantList list = it.value().toList();
            for (const QVariant &item : list) {
                items << item.toString();
            }
            if (!items.isEmpty()) {
                parts << items.join(QStringLiteral(", "));
            }
        } else {
            const QString value = it.value().toString();
            if (!value.isEmpty()) {
                parts << value;
            }
        }
    }

    QVariantMap patch;
    patch.insert(QStringLiteral("answered"), true);
    patch.insert(QStringLiteral("answers"), answers);
    patch.insert(QStringLiteral("answer"), parts.join(QStringLiteral("; ")));
    return patch;
}

QVariantMap toolApprovalPatch(const QString &decision)
{
    // Record the decision so the approval bar clears. A denial also flips the
    // tool to an error state; an approval leaves it running for the host to drive
    // to completion.
    QVariantMap patch;
    patch.insert(QStringLiteral("approval"), decision);
    patch.insert(QStringLiteral("needsApproval"), false);
    if (decision == QStringLiteral("denied")) {
        patch.insert(QStringLiteral("status"), QStringLiteral("error"));
    }
    return patch;
}

QVariantList ansiToSpans(const QString &text)
{
    QVariantList spans;

    int fg = -1;
    int bg = -1;
    bool bold = false;
    bool dim = false;
    bool italic = false;
    bool underline = false;
    bool inverse = false;

    QString run;
    const auto flush = [&]() {
        if (run.isEmpty()) {
            return;
        }
        QVariantMap span;
        span.insert(QStringLiteral("text"), run);
        span.insert(QStringLiteral("fg"), fg);
        span.insert(QStringLiteral("bg"), bg);
        span.insert(QStringLiteral("bold"), bold);
        span.insert(QStringLiteral("dim"), dim);
        span.insert(QStringLiteral("italic"), italic);
        span.insert(QStringLiteral("underline"), underline);
        span.insert(QStringLiteral("inverse"), inverse);
        spans.push_back(span);
        run.clear();
    };

    const auto applySgr = [&](const QString &params) {
        const QStringList parts = params.isEmpty() ? QStringList { QStringLiteral("0") }
                                                   : params.split(QLatin1Char(';'));
        for (int i = 0; i < parts.size(); ++i) {
            bool ok = false;
            const int code = parts.at(i).toInt(&ok);
            if (!ok) {
                continue;
            }
            if (code == 0) {
                fg = bg = -1;
                bold = dim = italic = underline = inverse = false;
            } else if (code == 1) {
                bold = true;
            } else if (code == 2) {
                dim = true;
            } else if (code == 3) {
                italic = true;
            } else if (code == 4) {
                underline = true;
            } else if (code == 7) {
                inverse = true;
            } else if (code == 22) {
                bold = dim = false;
            } else if (code == 23) {
                italic = false;
            } else if (code == 24) {
                underline = false;
            } else if (code == 27) {
                inverse = false;
            } else if (code >= 30 && code <= 37) {
                fg = code - 30;
            } else if (code == 39) {
                fg = -1;
            } else if (code >= 40 && code <= 47) {
                bg = code - 40;
            } else if (code == 49) {
                bg = -1;
            } else if (code >= 90 && code <= 97) {
                fg = 8 + (code - 90);
            } else if (code >= 100 && code <= 107) {
                bg = 8 + (code - 100);
            }
        }
    };

    const int n = text.size();
    int i = 0;
    while (i < n) {
        const QChar ch = text.at(i);
        if (ch == QChar(0x1B) && i + 1 < n && text.at(i + 1) == QLatin1Char('[')) {
            // CSI sequence: ESC [ <params> <final-byte>.
            int j = i + 2;
            QString params;
            while (j < n) {
                const QChar c = text.at(j);
                if ((c >= QLatin1Char('0') && c <= QLatin1Char('9')) || c == QLatin1Char(';')) {
                    params.append(c);
                    ++j;
                } else {
                    break;
                }
            }
            if (j < n) {
                const QChar final = text.at(j);
                if (final == QLatin1Char('m')) {
                    flush();
                    applySgr(params);
                }
                i = j + 1; // consume the final byte (drop non-SGR sequences too)
                continue;
            }
            // Unterminated escape: stop interpreting, keep the rest as text.
            run.append(text.mid(i));
            break;
        }
        run.append(ch);
        ++i;
    }
    flush();
    return spans;
}

QVariantList parseUnifiedDiff(const QString &diff)
{
    QVariantList out;
    const QStringList lines = diff.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        QString kind = QStringLiteral("context");
        if (line.startsWith(QStringLiteral("@@"))) {
            kind = QStringLiteral("hunk");
        } else if (line.startsWith(QStringLiteral("+++")) || line.startsWith(QStringLiteral("---"))
                   || line.startsWith(QStringLiteral("diff ")) || line.startsWith(QStringLiteral("index "))) {
            kind = QStringLiteral("meta");
        } else if (line.startsWith(QLatin1Char('+'))) {
            kind = QStringLiteral("add");
        } else if (line.startsWith(QLatin1Char('-'))) {
            kind = QStringLiteral("del");
        }
        QVariantMap entry;
        entry.insert(QStringLiteral("text"), line);
        entry.insert(QStringLiteral("kind"), kind);
        out.push_back(entry);
    }
    return out;
}

} // namespace be
