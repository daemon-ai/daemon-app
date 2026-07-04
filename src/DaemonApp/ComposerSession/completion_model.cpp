// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "completion_model.h"

namespace {

// The canned client-side data (no agent gateway is wired yet). Mirrors
// CompletionProvider.qml's slashCommands / mentions.
const QList<CompletionModel::Item>& slashCommands() {
    static const QList<CompletionModel::Item> pool = {
        {QStringLiteral("/new"), QObject::tr("Start a new session"),
         QObject::tr("Command", "completion group"), QStringLiteral("/new"), QStringLiteral("new")},
        {QStringLiteral("/theme"), QObject::tr("Open theme & settings"),
         QObject::tr("Command", "completion group"), QStringLiteral("/theme"),
         QStringLiteral("theme")},
        {QStringLiteral("/distraction"), QObject::tr("Toggle distraction-free"),
         QObject::tr("Command", "completion group"), QStringLiteral("/distraction"),
         QStringLiteral("distraction")},
        {QStringLiteral("/clear"), QObject::tr("Clear the composer"),
         QObject::tr("Command", "completion group"), QStringLiteral("/clear"),
         QStringLiteral("clear")},
        {QStringLiteral("/retry"), QObject::tr("Rewind & re-run the last message"),
         QObject::tr("Command", "completion group"), QStringLiteral("/retry"),
         QStringLiteral("retry")},
        {QStringLiteral("/edit"), QObject::tr("Rewind & edit the last message"),
         QObject::tr("Command", "completion group"), QStringLiteral("/edit"),
         QStringLiteral("edit")},
        {QStringLiteral("/undo"), QObject::tr("Undo the last exchange"),
         QObject::tr("Command", "completion group"), QStringLiteral("/undo"),
         QStringLiteral("undo")},
        {QStringLiteral("/model"), QObject::tr("Choose the active model"),
         QObject::tr("Command", "completion group"), QStringLiteral("/model"),
         QStringLiteral("model")},
        {QStringLiteral("/title"), QObject::tr("Rename this session"),
         QObject::tr("Command", "completion group"), QStringLiteral("/title"),
         QStringLiteral("title")},
        {QStringLiteral("/save"), QObject::tr("Export the transcript (JSON)"),
         QObject::tr("Command", "completion group"), QStringLiteral("/save"),
         QStringLiteral("save")},
        {QStringLiteral("/find"), QObject::tr("Search this transcript"),
         QObject::tr("Command", "completion group"), QStringLiteral("/find"),
         QStringLiteral("find")},
        {QStringLiteral("/usage"), QObject::tr("Show token usage & cost"),
         QObject::tr("Command", "completion group"), QStringLiteral("/usage"),
         QStringLiteral("usage")},
        {QStringLiteral("/compress"), QObject::tr("Compress the context window"),
         QObject::tr("Command", "completion group"), QStringLiteral("/compress"),
         QStringLiteral("compress")},
        {QStringLiteral("/help"), QObject::tr("Open the command palette"),
         QObject::tr("Command", "completion group"), QStringLiteral("/help"),
         QStringLiteral("help")},
        // Navigation: open an app-level manager page (mirrors the palette's
        // "Navigation" group; the host routes the action to the Nav seam).
        {QStringLiteral("/settings"), QObject::tr("Open settings"),
         QObject::tr("Go to", "completion group"), QStringLiteral("/settings"),
         QStringLiteral("settings")},
        {QStringLiteral("/dashboard"), QObject::tr("Open the dashboard"),
         QObject::tr("Go to", "completion group"), QStringLiteral("/dashboard"),
         QStringLiteral("dashboard")},
        {QStringLiteral("/models"), QObject::tr("Open the models hub"),
         QObject::tr("Go to", "completion group"), QStringLiteral("/models"),
         QStringLiteral("models")},
        {QStringLiteral("/accounts"), QObject::tr("Open accounts"),
         QObject::tr("Go to", "completion group"), QStringLiteral("/accounts"),
         QStringLiteral("accounts")},
        {QStringLiteral("/profiles"), QObject::tr("Open profiles"),
         QObject::tr("Go to", "completion group"), QStringLiteral("/profiles"),
         QStringLiteral("profiles")},
        {QStringLiteral("/fleet"), QObject::tr("Open the fleet view"),
         QObject::tr("Go to", "completion group"), QStringLiteral("/fleet"),
         QStringLiteral("fleet")},
        {QStringLiteral("/sessions"), QObject::tr("Open the sessions roster"),
         QObject::tr("Go to", "completion group"), QStringLiteral("/sessions"),
         QStringLiteral("sessions")},
        {QStringLiteral("/approvals"), QObject::tr("Open the approvals inbox"),
         QObject::tr("Go to", "completion group"), QStringLiteral("/approvals"),
         QStringLiteral("approvals")},
        {QStringLiteral("/routing"), QObject::tr("Open the routing matrix"),
         QObject::tr("Go to", "completion group"), QStringLiteral("/routing"),
         QStringLiteral("routing")},
        {QStringLiteral("/cron"), QObject::tr("Open scheduled jobs"),
         QObject::tr("Go to", "completion group"), QStringLiteral("/cron"), QStringLiteral("cron")},
    };
    return pool;
}

const QList<CompletionModel::Item>& mentions() {
    static const QList<CompletionModel::Item> pool = {
        {QStringLiteral("README.md"), QObject::tr("Project readme"),
         QObject::tr("File", "completion group"), QStringLiteral("@file:README.md "),
         QStringLiteral("insert")},
        {QStringLiteral("Composer.qml"), QObject::tr("Composer component"),
         QObject::tr("File", "completion group"), QStringLiteral("@file:Composer.qml "),
         QStringLiteral("insert")},
        {QStringLiteral("Theme.qml"), QObject::tr("Theme tokens"),
         QObject::tr("File", "completion group"), QStringLiteral("@file:Theme.qml "),
         QStringLiteral("insert")},
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
