#include "diagram/parser/flowchart_parser.h"

#include <QRegularExpression>
#include <QStringList>

namespace be::diagram {

namespace {

struct ShapeDelim {
    QString open;
    QString close;
    NodeShape shape;
};

// Ordered longest-open-first so two-char delimiters win over one-char ones.
const QVector<ShapeDelim>& shapeDelims() {
    static const QVector<ShapeDelim> delims = {
        {QStringLiteral("(("), QStringLiteral("))"), NodeShape::Circle},
        {QStringLiteral("(["), QStringLiteral("])"), NodeShape::Stadium},
        {QStringLiteral("[["), QStringLiteral("]]"), NodeShape::Subroutine},
        {QStringLiteral("[("), QStringLiteral(")]"), NodeShape::Cylinder},
        {QStringLiteral("{{"), QStringLiteral("}}"), NodeShape::Hexagon},
        {QStringLiteral("["), QStringLiteral("]"), NodeShape::Rect},
        {QStringLiteral("("), QStringLiteral(")"), NodeShape::RoundRect},
        {QStringLiteral("{"), QStringLiteral("}"), NodeShape::Diamond},
    };
    return delims;
}

QString unquote(QString s) {
    s = s.trimmed();
    if (s.size() >= 2 && s.startsWith(QLatin1Char('"')) && s.endsWith(QLatin1Char('"'))) {
        s = s.mid(1, s.size() - 2);
    }
    return s;
}

bool isIdChar(QChar c) {
    // Mermaid node ids are alphanumeric + underscore; '-'/'.' are reserved for
    // connectors so they must not be consumed as part of an id.
    return c.isLetterOrNumber() || c == QLatin1Char('_');
}

// Strip %% comments (whole line or trailing) and trim.
QString stripComment(const QString& line) {
    const qsizetype idx = line.indexOf(QStringLiteral("%%"));
    return (idx < 0 ? line : line.left(idx));
}

// Rewrite the inline-label edge form `A -- text --> B` into `A -->|text| B` so
// the tokenizer only has to deal with operator + optional |label|.
QString normalizeInlineLabels(QString stmt) {
    static const QRegularExpression mid(
        QStringLiteral("(--|==|-\\.)\\s+([^|>\\n]+?)\\s+(-->|---|-\\.->|==>|===)"));
    QString out;
    qsizetype last = 0;
    auto it = mid.globalMatch(stmt);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        out += stmt.mid(last, m.capturedStart() - last);
        out += m.captured(3) + QLatin1Char('|') + m.captured(2).trimmed() + QLatin1Char('|');
        last = m.capturedEnd();
    }
    out += stmt.mid(last);
    return out;
}

struct ParsedNodeRef {
    QString id;
    QString label;
    NodeShape shape = NodeShape::Rect;
    bool hasShape = false;
    QString classRef;
    bool valid = false;
    qsizetype next = 0;
};

ParsedNodeRef parseNodeRef(const QString& s, qsizetype pos) {
    ParsedNodeRef ref;
    while (pos < s.size() && s.at(pos).isSpace()) {
        ++pos;
    }
    const qsizetype idStart = pos;
    while (pos < s.size() && isIdChar(s.at(pos))) {
        ++pos;
    }
    if (pos == idStart) {
        ref.next = pos;
        return ref;
    }
    ref.id = s.mid(idStart, pos - idStart);
    ref.label = ref.id;
    ref.valid = true;

    // Optional shape with label.
    for (const ShapeDelim& d : shapeDelims()) {
        if (s.mid(pos, d.open.size()) == d.open) {
            const qsizetype contentStart = pos + d.open.size();
            const qsizetype closeAt = s.indexOf(d.close, contentStart);
            if (closeAt >= 0) {
                ref.label = unquote(s.mid(contentStart, closeAt - contentStart));
                ref.shape = d.shape;
                ref.hasShape = true;
                pos = closeAt + d.close.size();
            }
            break;
        }
    }

    // Optional inline class `:::name`.
    if (s.mid(pos, 3) == QStringLiteral(":::")) {
        qsizetype cs = pos + 3;
        const qsizetype clsStart = cs;
        while (cs < s.size() && isIdChar(s.at(cs))) {
            ++cs;
        }
        ref.classRef = s.mid(clsStart, cs - clsStart);
        pos = cs;
    }

    ref.next = pos;
    return ref;
}

struct ParsedConnector {
    bool valid = false;
    EdgeKind kind = EdgeKind::Normal;
    ArrowHead head = ArrowHead::Arrow;
    QString label;
    qsizetype next = 0;
};

ParsedConnector parseConnector(const QString& s, qsizetype pos) {
    ParsedConnector c;
    while (pos < s.size() && s.at(pos).isSpace()) {
        ++pos;
    }
    static const QVector<QString> ops = {
        QStringLiteral("-.->"), QStringLiteral("-.-"), QStringLiteral("-->"),
        QStringLiteral("---"),  QStringLiteral("==>"), QStringLiteral("==="),
    };
    QString matched;
    for (const QString& op : ops) {
        if (s.mid(pos, op.size()) == op) {
            matched = op;
            break;
        }
    }
    if (matched.isEmpty()) {
        c.next = pos;
        return c;
    }
    pos += matched.size();
    c.valid = true;
    c.kind = matched.contains(QLatin1Char('.'))   ? EdgeKind::Dotted
             : matched.contains(QLatin1Char('=')) ? EdgeKind::Thick
                                                  : EdgeKind::Normal;
    c.head = matched.endsWith(QLatin1Char('>')) ? ArrowHead::Arrow : ArrowHead::None;

    // Optional |label|.
    while (pos < s.size() && s.at(pos).isSpace()) {
        ++pos;
    }
    if (pos < s.size() && s.at(pos) == QLatin1Char('|')) {
        const qsizetype labelStart = pos + 1;
        const qsizetype labelEnd = s.indexOf(QLatin1Char('|'), labelStart);
        if (labelEnd >= 0) {
            c.label = unquote(s.mid(labelStart, labelEnd - labelStart));
            pos = labelEnd + 1;
        }
    }
    c.next = pos;
    return c;
}

class Builder {
public:
    DiagramModel model;

    DiagramNode& ensureNode(const QString& id) {
        int idx = model.nodeIndex.value(id, -1);
        if (idx < 0) {
            DiagramNode node;
            node.id = id;
            node.label = id;
            model.nodes.push_back(node);
            idx = model.nodes.size() - 1;
            model.nodeIndex.insert(id, idx);
        }
        // Assign cluster membership on the first reference seen inside a subgraph,
        // even if the node was first introduced outside it (mermaid semantics).
        if (!m_clusterStack.isEmpty() && model.nodes[idx].parentCluster.isEmpty()) {
            DiagramCluster& cluster = model.clusters[m_clusterStack.last()];
            cluster.memberIds.push_back(id);
            model.nodes[idx].parentCluster = cluster.id;
        }
        return model.nodes[idx];
    }

    DiagramCluster* currentCluster() {
        return m_clusterStack.isEmpty() ? nullptr : &model.clusters[m_clusterStack.last()];
    }

    void applyRef(const ParsedNodeRef& ref) {
        DiagramNode& node = ensureNode(ref.id);
        if (ref.hasShape) {
            node.label = ref.label;
            node.shape = ref.shape;
        }
        if (!ref.classRef.isEmpty()) {
            node.classRef = ref.classRef;
        }
    }

    void pushCluster(const QString& id, const QString& title) {
        DiagramCluster cluster;
        cluster.id = id.isEmpty() ? QStringLiteral("sg%1").arg(model.clusters.size()) : id;
        cluster.title = title;
        if (!m_clusterStack.isEmpty()) {
            cluster.parentId = model.clusters[m_clusterStack.last()].id;
        }
        model.clusters.push_back(cluster);
        m_clusterStack.push_back(model.clusters.size() - 1);
    }

    void popCluster() {
        if (!m_clusterStack.isEmpty()) {
            m_clusterStack.removeLast();
        }
    }

private:
    // Indices into model.clusters (indices stay valid across vector growth,
    // unlike pointers, which would dangle when a nested subgraph reallocates).
    QVector<int> m_clusterStack;
};

Direction parseDirection(const QString& token) {
    const QString t = token.toUpper();
    if (t == QStringLiteral("LR")) {
        return Direction::LR;
    }
    if (t == QStringLiteral("RL")) {
        return Direction::RL;
    }
    if (t == QStringLiteral("BT")) {
        return Direction::BT;
    }
    return Direction::TB; // TD / TB / default
}

} // namespace

DiagramModel parseFlowchart(const QString& source) {
    Builder builder;
    DiagramModel& model = builder.model;
    model.family = QStringLiteral("flowchart");

    // Split into statements on newlines and ';'.
    QStringList rawLines;
    for (const QString& line : source.split(QLatin1Char('\n'))) {
        for (const QString& part : stripComment(line).split(QLatin1Char(';'))) {
            rawLines.push_back(part);
        }
    }

    bool headerSeen = false;
    for (const QString& raw : rawLines) {
        const QString stmt = raw.trimmed();
        if (stmt.isEmpty()) {
            continue;
        }

        if (!headerSeen) {
            static const QRegularExpression headerRe(
                QStringLiteral("^(flowchart|graph)\\s*([A-Za-z]{2})?"));
            const QRegularExpressionMatch m = headerRe.match(stmt);
            if (m.hasMatch()) {
                headerSeen = true;
                if (!m.captured(2).isEmpty()) {
                    model.dir = parseDirection(m.captured(2));
                }
                continue;
            }
            // Tolerate a missing header; assume flowchart TB.
            headerSeen = true;
        }

        // Structural keywords.
        if (stmt == QStringLiteral("end")) {
            builder.popCluster();
            continue;
        }
        if (stmt.startsWith(QStringLiteral("subgraph"))) {
            const QString rest = stmt.mid(QStringLiteral("subgraph").size()).trimmed();
            QString id = rest;
            QString title = rest;
            static const QRegularExpression titleRe(QStringLiteral("^(\\S+)\\s*\\[(.*)\\]$"));
            const QRegularExpressionMatch tm = titleRe.match(rest);
            if (tm.hasMatch()) {
                id = tm.captured(1);
                title = unquote(tm.captured(2));
            }
            builder.pushCluster(id, title);
            continue;
        }
        if (stmt.startsWith(QStringLiteral("direction"))) {
            const Direction d =
                parseDirection(stmt.mid(QStringLiteral("direction").size()).trimmed());
            if (DiagramCluster* cluster = builder.currentCluster()) {
                cluster->dir = d;
                cluster->dirSet = true;
            } else {
                model.dir = d;
            }
            continue;
        }
        if (stmt.startsWith(QStringLiteral("classDef"))) {
            const QString rest = stmt.mid(QStringLiteral("classDef").size()).trimmed();
            const qsizetype sp = rest.indexOf(QLatin1Char(' '));
            if (sp > 0) {
                const QString name = rest.left(sp);
                QHash<QString, QString> props;
                for (const QString& kv : rest.mid(sp + 1).split(QLatin1Char(','))) {
                    const qsizetype colon = kv.indexOf(QLatin1Char(':'));
                    if (colon > 0) {
                        props.insert(kv.left(colon).trimmed(), kv.mid(colon + 1).trimmed());
                    }
                }
                model.classDefs.insert(name, props);
            }
            continue;
        }
        if (stmt.startsWith(QStringLiteral("class "))) {
            const QString rest = stmt.mid(QStringLiteral("class").size()).trimmed();
            const qsizetype sp = rest.lastIndexOf(QLatin1Char(' '));
            if (sp > 0) {
                const QString name = rest.mid(sp + 1).trimmed();
                for (const QString& id : rest.left(sp).split(QLatin1Char(','))) {
                    DiagramNode& node = builder.ensureNode(id.trimmed());
                    node.classRef = name;
                }
            }
            continue;
        }

        // Edge / node chain.
        const QString normalized = normalizeInlineLabels(stmt);
        qsizetype pos = 0;
        ParsedNodeRef prev = parseNodeRef(normalized, pos);
        if (!prev.valid) {
            continue;
        }
        builder.applyRef(prev);
        pos = prev.next;

        while (true) {
            const ParsedConnector conn = parseConnector(normalized, pos);
            if (!conn.valid) {
                break;
            }
            pos = conn.next;
            const ParsedNodeRef cur = parseNodeRef(normalized, pos);
            if (!cur.valid) {
                break;
            }
            builder.applyRef(cur);
            pos = cur.next;

            DiagramEdge edge;
            edge.fromId = prev.id;
            edge.toId = cur.id;
            edge.kind = conn.kind;
            edge.head = conn.head;
            edge.label = conn.label;
            model.edges.push_back(edge);

            prev = cur;
        }
    }

    model.valid = !model.nodes.isEmpty();
    if (!model.valid) {
        model.error = QStringLiteral("No nodes found");
    }
    return model;
}

} // namespace be::diagram
