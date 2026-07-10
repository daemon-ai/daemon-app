#include "mirror/conversation_list_model.h"

namespace mirror {

ConversationListModel::ConversationListModel(Store& store, QObject* parent)
    : TableModel<Conversation>(store, parent) {
    // This leaf overrides lessThan (presentation sort), so re-derive the initial order with the
    // fully-constructed vtable (silent, construction-time — no reset signal, §14.8).
    reprime();
}

QHash<int, QByteArray> ConversationListModel::roleNames() const {
    QHash<int, QByteArray> names;
    names.insert(StableIdRole, QByteArrayLiteral("stableId"));
    names.insert(TransportRole, QByteArrayLiteral("transport"));
    names.insert(IdRole, QByteArrayLiteral("conversationId"));
    names.insert(TitleRole, QByteArrayLiteral("title"));
    names.insert(TopicRole, QByteArrayLiteral("topic"));
    names.insert(MemberCountRole, QByteArrayLiteral("memberCount"));
    return names;
}

QVariant ConversationListModel::roleData(const Conversation& c, int role) const {
    switch (role) {
    case TransportRole:
        return c.transport;
    case IdRole:
        return c.id;
    case TitleRole:
        return c.title;
    case TopicRole:
        return c.topic;
    case MemberCountRole:
        return c.member_count;
    default:
        return {};
    }
}

bool ConversationListModel::lessThan(const Conversation& a, const Conversation& b) const {
    return a.title.compare(b.title, Qt::CaseInsensitive) < 0;
}

} // namespace mirror
