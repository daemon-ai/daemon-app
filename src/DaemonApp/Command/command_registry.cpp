#include "command_registry.h"

CommandRegistry::CommandRegistry(QObject* parent)
    : QAbstractListModel(parent)
{
    // The curated, client-satisfiable catalog. ids match the slash-command verbs
    // and front-end intents the two shells already route; the daemon can extend
    // this later without touching either UI.
    m_all = {
        { QStringLiteral("new"), tr("New session"), tr("Session"),
          tr("Start a fresh chat"), QStringLiteral("Ctrl+T") },
        { QStringLiteral("search"), tr("Search sessions"), tr("Navigation"),
          tr("Jump to a session"), QString() },
        { QStringLiteral("settings"), tr("Open settings"), tr("Navigation"), QString(),
          QString() },
        { QStringLiteral("files"), tr("Toggle file explorer"), tr("Navigation"),
          tr("Show / hide the workspace file tree"), QStringLiteral("Ctrl+E") },
        // App-level manager pages (GUI: Nav overlay; TUI: page tabs). ids match
        // the front-end route strings both shells already understand.
        { QStringLiteral("dashboard"), tr("Open dashboard"), tr("Navigation"),
          tr("Sessions, agents, approvals overview"), QString() },
        { QStringLiteral("models"), tr("Open models hub"), tr("Navigation"),
          tr("Discover / download / installed models"), QString() },
        { QStringLiteral("accounts"), tr("Open accounts"), tr("Navigation"),
          tr("Provider sign-in + API keys"), QString() },
        { QStringLiteral("profiles"), tr("Open profiles"), tr("Navigation"),
          tr("Agent profiles + skills/tools"), QString() },
        { QStringLiteral("fleet"), tr("Open fleet"), tr("Navigation"),
          tr("Orchestrator / worker tree"), QString() },
        { QStringLiteral("sessions"), tr("Open sessions"), tr("Navigation"),
          tr("Durable + live sessions"), QString() },
        { QStringLiteral("approvals"), tr("Open approvals"), tr("Navigation"),
          tr("Pending tool approvals"), QString() },
        { QStringLiteral("routing"), tr("Open routing"), tr("Navigation"),
          tr("Intent \u2192 model rules"), QString() },
        { QStringLiteral("cron"), tr("Open scheduled jobs"), tr("Navigation"),
          tr("Cron-style scheduled jobs"), QString() },
        { QStringLiteral("theme"), tr("Cycle theme"), tr("Appearance"),
          tr("Light / Dark / Sepia / Midnight"), QStringLiteral("F8") },
        { QStringLiteral("model"), tr("Choose model\u2026"), tr("Composer"),
          tr("Pick the active model"), QString() },
        { QStringLiteral("reasoning"), tr("Cycle reasoning effort"), tr("Composer"),
          tr("off / low / medium / high"), QString() },
        { QStringLiteral("fast"), tr("Toggle fast mode"), tr("Composer"), QString(), QString() },
        { QStringLiteral("verbose"), tr("Toggle verbose"), tr("Composer"), QString(), QString() },
        { QStringLiteral("usage"), tr("Show usage"), tr("Session"),
          tr("Tokens + spend this session"), QString() },
        { QStringLiteral("compress"), tr("Compress context"), tr("Session"),
          tr("Summarize + free the window"), QString() },
        { QStringLiteral("save"), tr("Export transcript"), tr("Session"),
          tr("Write the session to JSON"), QString() },
        { QStringLiteral("find"), tr("Find in transcript"), tr("Navigation"),
          tr("Search this session"), QStringLiteral("Ctrl+F") },
        { QStringLiteral("title"), tr("Rename session"), tr("Session"), QString(),
          QString() },
        { QStringLiteral("retry"), tr("Retry last message"), tr("Rewind"), QString(), QString() },
        { QStringLiteral("edit"), tr("Edit last message"), tr("Rewind"), QString(), QString() },
        { QStringLiteral("undo"), tr("Undo last exchange"), tr("Rewind"), QString(), QString() },
        { QStringLiteral("clear"), tr("Clear session"), tr("Session"),
          tr("Remove all messages"), QString() },
        { QStringLiteral("help"), tr("Help"), tr("Navigation"), tr("Commands + shortcuts"),
          QString() },
        { QStringLiteral("distraction"), tr("Distraction-free mode"), tr("Appearance"), QString(),
          QStringLiteral("Esc to exit") },
    };
    applyFilter(QString());
}

int CommandRegistry::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_filtered.size());
}

QVariant CommandRegistry::data(const QModelIndex& index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= m_filtered.size()) {
        return {};
    }
    const Command& c = m_all.at(m_filtered.at(row));
    switch (role) {
    case IdRole:
        return c.id;
    case TitleRole:
        return c.title;
    case GroupRole:
        return c.group;
    case HintRole:
        return c.hint;
    case ShortcutRole:
        return c.shortcut;
    default:
        return {};
    }
}

QHash<int, QByteArray> CommandRegistry::roleNames() const
{
    return {
        { IdRole, "commandId" },
        { TitleRole, "title" },
        { GroupRole, "group" },
        { HintRole, "hint" },
        { ShortcutRole, "shortcut" },
    };
}

void CommandRegistry::search(const QString& query)
{
    if (m_query == query) {
        return;
    }
    beginResetModel();
    applyFilter(query);
    endResetModel();
    emit countChanged();
}

void CommandRegistry::applyFilter(const QString& query)
{
    m_query = query;
    const QString q = query.trimmed().toLower();
    m_filtered.clear();
    for (int i = 0; i < m_all.size(); ++i) {
        const Command& c = m_all.at(i);
        if (q.isEmpty()) {
            m_filtered.append(i);
            continue;
        }
        const QString hay = (c.title + QStringLiteral(" ") + c.hint + QStringLiteral(" ") + c.id
                             + QStringLiteral(" ") + c.group)
                                .toLower();
        if (hay.contains(q)) {
            m_filtered.append(i);
        }
    }
}

void CommandRegistry::trigger(int row)
{
    if (row < 0 || row >= m_filtered.size()) {
        return;
    }
    emit commandTriggered(m_all.at(m_filtered.at(row)).id);
}

void CommandRegistry::triggerId(const QString& id)
{
    emit commandTriggered(id);
}

QString CommandRegistry::idAt(int row) const
{
    if (row < 0 || row >= m_filtered.size()) {
        return {};
    }
    return m_all.at(m_filtered.at(row)).id;
}

QVector<CommandRegistry::Command> CommandRegistry::visibleCommands() const
{
    QVector<Command> out;
    out.reserve(m_filtered.size());
    for (int idx : m_filtered) {
        out.append(m_all.at(idx));
    }
    return out;
}
