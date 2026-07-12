#pragma once
// mirror::ConversationListModel — the per-VM role-map DEMONSTRATOR (spec 09 §8.2/§8.3).
//
// This is the ~30-60-line pattern a concrete consumer contributes on top of the generic
// TableModel<Entity>: a curated roleNames() + a data() switch reading typed fields straight off
// the generated Q_GADGET entity (no QVariantMap row shape, §14.10), plus an optional presentation
// sort (title, case-insensitive). It is a PATTERN DEMONSTRATOR WITH TESTS, not a consumer
// migration — the real conversation/sidebar surfaces port onto this shape in the M2 wave (A5),
// wiring GUI (QML) + TUI (DisplayRoleAdapter) then. It is deliberately GUI-free C++ (§8.4).

#include "mirror/generated/entities_gen.h"
#include "mirror/table_model.h"

#include <QByteArray>
#include <QHash>
#include <QVariant>

namespace mirror {

class ConversationListModel : public TableModel<Conversation> {
    Q_OBJECT

public:
    enum Roles : int {
        TransportRole = Qt::UserRole + 100,
        IdRole,
        TitleRole,
        TopicRole,
        MemberCountRole,
    };

    explicit ConversationListModel(Store& store, QObject* parent = nullptr);

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

protected:
    [[nodiscard]] QVariant roleData(const Conversation& c, int role) const override;
    // Presentation order: title (case-insensitive), TableModel's identity tiebreak keeps it total.
    [[nodiscard]] bool lessThan(const Conversation& a, const Conversation& b) const override;
};

} // namespace mirror
