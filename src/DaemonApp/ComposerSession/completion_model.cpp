#include "completion_model.h"

namespace {

// The canned client-side data (no agent gateway is wired yet). Mirrors
// CompletionProvider.qml's slashCommands / mentions.
const QList<CompletionModel::Item>& slashCommands() {
    static const QList<CompletionModel::Item> pool = {
        {QStringLiteral("/new"), QObject::tr("Start a new session"), QStringLiteral("Command"),
         QStringLiteral("/new"), QStringLiteral("new")},
        {QStringLiteral("/theme"), QObject::tr("Open theme & settings"), QStringLiteral("Command"),
         QStringLiteral("/theme"), QStringLiteral("theme")},
        {QStringLiteral("/distraction"), QObject::tr("Toggle distraction-free"),
         QStringLiteral("Command"), QStringLiteral("/distraction"), QStringLiteral("distraction")},
        {QStringLiteral("/clear"), QObject::tr("Clear the composer"), QStringLiteral("Command"),
         QStringLiteral("/clear"), QStringLiteral("clear")},
        {QStringLiteral("/retry"), QObject::tr("Rewind & re-run the last message"),
         QStringLiteral("Command"), QStringLiteral("/retry"), QStringLiteral("retry")},
        {QStringLiteral("/edit"), QObject::tr("Rewind & edit the last message"),
         QStringLiteral("Command"), QStringLiteral("/edit"), QStringLiteral("edit")},
        {QStringLiteral("/undo"), QObject::tr("Undo the last exchange"), QStringLiteral("Command"),
         QStringLiteral("/undo"), QStringLiteral("undo")},
        {QStringLiteral("/model"), QObject::tr("Choose the active model"),
         QStringLiteral("Command"), QStringLiteral("/model"), QStringLiteral("model")},
        {QStringLiteral("/title"), QObject::tr("Rename this session"), QStringLiteral("Command"),
         QStringLiteral("/title"), QStringLiteral("title")},
        {QStringLiteral("/save"), QObject::tr("Export the transcript (JSON)"),
         QStringLiteral("Command"), QStringLiteral("/save"), QStringLiteral("save")},
        {QStringLiteral("/find"), QObject::tr("Search this transcript"), QStringLiteral("Command"),
         QStringLiteral("/find"), QStringLiteral("find")},
        {QStringLiteral("/usage"), QObject::tr("Show token usage & cost"),
         QStringLiteral("Command"), QStringLiteral("/usage"), QStringLiteral("usage")},
        {QStringLiteral("/compress"), QObject::tr("Compress the context window"),
         QStringLiteral("Command"), QStringLiteral("/compress"), QStringLiteral("compress")},
        {QStringLiteral("/help"), QObject::tr("Open the command palette"),
         QStringLiteral("Command"), QStringLiteral("/help"), QStringLiteral("help")},
        // Navigation: open an app-level manager page (mirrors the palette's
        // "Navigation" group; the host routes the action to the Nav seam).
        {QStringLiteral("/settings"), QObject::tr("Open settings"), QStringLiteral("Go to"),
         QStringLiteral("/settings"), QStringLiteral("settings")},
        {QStringLiteral("/dashboard"), QObject::tr("Open the dashboard"), QStringLiteral("Go to"),
         QStringLiteral("/dashboard"), QStringLiteral("dashboard")},
        {QStringLiteral("/models"), QObject::tr("Open the models hub"), QStringLiteral("Go to"),
         QStringLiteral("/models"), QStringLiteral("models")},
        {QStringLiteral("/accounts"), QObject::tr("Open accounts"), QStringLiteral("Go to"),
         QStringLiteral("/accounts"), QStringLiteral("accounts")},
        {QStringLiteral("/profiles"), QObject::tr("Open profiles"), QStringLiteral("Go to"),
         QStringLiteral("/profiles"), QStringLiteral("profiles")},
        {QStringLiteral("/fleet"), QObject::tr("Open the fleet view"), QStringLiteral("Go to"),
         QStringLiteral("/fleet"), QStringLiteral("fleet")},
        {QStringLiteral("/sessions"), QObject::tr("Open the sessions roster"),
         QStringLiteral("Go to"), QStringLiteral("/sessions"), QStringLiteral("sessions")},
        {QStringLiteral("/approvals"), QObject::tr("Open the approvals inbox"),
         QStringLiteral("Go to"), QStringLiteral("/approvals"), QStringLiteral("approvals")},
        {QStringLiteral("/routing"), QObject::tr("Open the routing matrix"),
         QStringLiteral("Go to"), QStringLiteral("/routing"), QStringLiteral("routing")},
        {QStringLiteral("/cron"), QObject::tr("Open scheduled jobs"), QStringLiteral("Go to"),
         QStringLiteral("/cron"), QStringLiteral("cron")},
    };
    return pool;
}

const QList<CompletionModel::Item>& mentions() {
    static const QList<CompletionModel::Item> pool = {
        {QStringLiteral("README.md"), QObject::tr("Project readme"), QStringLiteral("File"),
         QStringLiteral("@file:README.md "), QStringLiteral("insert")},
        {QStringLiteral("Composer.qml"), QObject::tr("Composer component"), QStringLiteral("File"),
         QStringLiteral("@file:Composer.qml "), QStringLiteral("insert")},
        {QStringLiteral("Theme.qml"), QObject::tr("Theme tokens"), QStringLiteral("File"),
         QStringLiteral("@file:Theme.qml "), QStringLiteral("insert")},
    };
    return pool;
}

} // namespace

CompletionModel::CompletionModel(QObject* parent) : QAbstractListModel(parent) {}

int CompletionModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_items.size());
}

QVariant CompletionModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }
    const Item& item = m_items.at(index.row());
    switch (role) {
    case LabelRole:
        return item.label;
    case HintRole:
        return item.hint;
    case GroupRole:
        return item.group;
    case ValueRole:
        return item.value;
    case ActionRole:
        return item.action;
    default:
        return {};
    }
}

QHash<int, QByteArray> CompletionModel::roleNames() const {
    return {
        {LabelRole, QByteArrayLiteral("label")},   {HintRole, QByteArrayLiteral("hint")},
        {GroupRole, QByteArrayLiteral("group")},   {ValueRole, QByteArrayLiteral("value")},
        {ActionRole, QByteArrayLiteral("action")},
    };
}

void CompletionModel::setItems(const QList<Item>& items) {
    beginResetModel();
    m_items = items;
    endResetModel();
    emit countChanged();
}

CompletionModel::Item CompletionModel::at(int index) const {
    if (index < 0 || index >= m_items.size()) {
        return {};
    }
    return m_items.at(index);
}

QList<CompletionModel::Item> CompletionModel::search(const QString& kind, const QString& query) {
    const QList<Item>& pool = (kind == QStringLiteral("mention")) ? mentions() : slashCommands();
    const QString q = query.toLower();

    QList<Item> out;
    if (q.isEmpty()) {
        out = pool;
    } else {
        for (const Item& item : pool) {
            if (item.label.toLower().contains(q)) {
                out.push_back(item);
            }
        }
    }
    if (out.size() > 8) {
        out = out.mid(0, 8);
    }
    return out;
}
