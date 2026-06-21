#pragma once

#include <QIdentityProxyModel>

// Bridges the app's custom-role list models to what Tui::ZListView reads.
//
// The GUI models (SidebarModel, ConversationsListModel) were written for QML,
// which addresses data by role *name*, so their human-readable text lives under
// custom roles (LabelRole / TitleRole) and Qt::DisplayRole is unused.
// Tui::ZListView, by contrast, paints Qt::DisplayRole plus an optional colored
// glyph via Tui's LeftDecoration* roles. This proxy maps the former onto the
// latter without changing a line of model code - the only glue the port needs.
class DisplayRoleAdapter : public QIdentityProxyModel {
    Q_OBJECT

public:
    enum class Kind {
        Sidebar,          // flattened agent tree (depth indent + tag/state dots)
        ConversationList, // conversations for the current scope
    };

    explicit DisplayRoleAdapter(Kind kind, QObject* parent = nullptr);

    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;

private:
    [[nodiscard]] QVariant sidebarData(const QModelIndex& src, int role) const;
    [[nodiscard]] QVariant listData(const QModelIndex& src, int role) const;

    Kind m_kind;
};
