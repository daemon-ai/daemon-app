// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "display_role_adapter.h"

#include "domain/sidebar_node.h"
#include "integrations_tree_model.h"
#include "presentation/display_presenter.h"
#include "sessions_list_model.h"
#include "sidebar_model.h"
#include "tui_palette.h"

#include <Tui/ZColor.h>
#include <Tui/ZCommon.h>

namespace {

// Map a "#rrggbb" token (the colors the models carry, mirroring Theme.qml) to a
// Tui RGB color for a list-row decoration glyph. Parsed by hand so the TUI need
// not link QtGui just for QColor.
Tui::ZColor rgbFromHex(const QString& hex) {
    if (hex.size() == 7 && hex.startsWith(QLatin1Char('#'))) {
        bool ok = false;
        const int rgb = hex.mid(1).toInt(&ok, 16);
        if (ok) {
            return {(rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff};
        }
    }
    return {0xaa, 0xaa, 0xaa};
}

} // namespace

DisplayRoleAdapter::DisplayRoleAdapter(Kind kind, QObject* parent)
    : QIdentityProxyModel(parent), m_kind(kind) {}

QVariant DisplayRoleAdapter::data(const QModelIndex& index, int role) const {
    if (!sourceModel() || !index.isValid()) {
        return {};
    }
    const QModelIndex src = mapToSource(index);
    switch (m_kind) {
    case Kind::Sidebar:
        return sidebarData(src, role);
    case Kind::Integrations:
        return integrationsData(src, role);
    case Kind::SessionList:
        break;
    }
    return listData(src, role);
}

QVariant DisplayRoleAdapter::sidebarData(const QModelIndex& src, int role) const {
    const auto srcData = [&](int r) { return sourceModel()->data(src, r); };

    if (role == Qt::DisplayRole) {
        const QString label = srcData(SidebarModel::LabelRole).toString();
        if (srcData(SidebarModel::IsSeparatorRole).toBool()) {
            // Collapsible section headers (Fleet / Tags / Integrations) carry a
            // disclosure glyph; plain dividers (none today) render without one.
            if (srcData(SidebarModel::HasChildrenRole).toBool()) {
                const QString tw = srcData(SidebarModel::ExpandedRole).toBool()
                                       ? QStringLiteral("\u25be ")
                                       : QStringLiteral("\u25b8 ");
                return QStringLiteral("%1\u2500\u2500 %2 \u2500\u2500").arg(tw, label);
            }
            return QStringLiteral("\u2500\u2500 %1 \u2500\u2500").arg(label);
        }

        QString text;
        const int depth = srcData(SidebarModel::DepthRole).toInt();
        text += QString(static_cast<qsizetype>(depth) * 2, QLatin1Char(' '));

        if (srcData(SidebarModel::HasChildrenRole).toBool()) {
            text += srcData(SidebarModel::ExpandedRole).toBool() ? QStringLiteral("\u25be ")
                                                                 : QStringLiteral("\u25b8 ");
        }
        text += label;

        // Integrations rows carry an inline session title ("#secops -> triage");
        // their member count is already conveyed by that sublabel.
        const int nodeTypeForLabel = srcData(SidebarModel::NodeTypeRole).toInt();
        if (nodeTypeForLabel == static_cast<int>(domain::NodeType::Transport)) {
            const QString sub = srcData(SidebarModel::SubLabelRole).toString();
            if (!sub.isEmpty()) {
                text += QStringLiteral("  \u2192 %1").arg(sub);
            }
            return text;
        }

        const int count = srcData(SidebarModel::CountRole).toInt();
        if (count >= 0) {
            text += QStringLiteral("  (%1)").arg(count);
        }
        return text;
    }

    // Colored dot in front of tag rows (and a state pip for agent nodes), proving
    // the model's color/state roles map onto Tui list decorations.
    const int nodeType = srcData(SidebarModel::NodeTypeRole).toInt();
    if (role == Tui::LeftDecorationRole) {
        if (nodeType == static_cast<int>(domain::NodeType::Tag)) {
            return QStringLiteral("\u25cf");
        }
        if (nodeType == static_cast<int>(domain::NodeType::Unit)) {
            return QStringLiteral("\u2022");
        }
        // A presence dot on a transport account (the events-IO instance).
        if (nodeType == static_cast<int>(domain::NodeType::Transport) &&
            srcData(SidebarModel::TxKindRole).toString() == QStringLiteral("account") &&
            !srcData(SidebarModel::PresenceRole).toString().isEmpty()) {
            return QStringLiteral("\u25cf");
        }
        return {};
    }
    if (role == Tui::LeftDecorationFgRole) {
        if (nodeType == static_cast<int>(domain::NodeType::Tag)) {
            return QVariant::fromValue(rgbFromHex(srcData(SidebarModel::ColorRole).toString()));
        }
        if (nodeType == static_cast<int>(domain::NodeType::Transport)) {
            return srcData(SidebarModel::PresenceRole).toString() == QStringLiteral("available")
                       ? QVariant::fromValue(tpal::statusOk())
                       : QVariant::fromValue(tpal::muted());
        }
        if (nodeType == static_cast<int>(domain::NodeType::Unit)) {
            // Reuse the shared C++ categorization (same source the GUI sidebar
            // uses), then resolve the semantic tone to a Tui color here.
            const int state = srcData(SidebarModel::StateRole).toInt();
            // Resolve the semantic tone to a theme-aware palette color (instead of
            // raw Tui::Colors) so the pips track the active theme like the rest of
            // the chrome.
            switch (DisplayPresenter::agentStateToneFor(state)) {
            case DisplayPresenter::StateTone::Running:
                return QVariant::fromValue(tpal::statusOk());
            case DisplayPresenter::StateTone::Finished:
                return QVariant::fromValue(tpal::muted());
            case DisplayPresenter::StateTone::Neutral:
                return QVariant::fromValue(tpal::warn());
            }
        }
        return {};
    }
    if (role == Tui::LeftDecorationSpaceRole) {
        return 1;
    }

    return sourceModel()->data(src, role);
}

QVariant DisplayRoleAdapter::integrationsData(const QModelIndex& src, int role) const {
    const auto srcData = [&](int r) { return sourceModel()->data(src, r); };

    if (role == Qt::DisplayRole) {
        const QString label = srcData(IntegrationsTreeModel::LabelRole).toString();
        const QString rowKind = srcData(IntegrationsTreeModel::RowKindRole).toString();
        const int depth = srcData(IntegrationsTreeModel::DepthRole).toInt();

        // Section headers (Persons / Channels / Direct Messages) render as a foldable divider.
        if (rowKind == QStringLiteral("section")) {
            const QString tw = srcData(IntegrationsTreeModel::ExpandedRole).toBool()
                                   ? QStringLiteral("\u25be ")
                                   : QStringLiteral("\u25b8 ");
            const QString indent(static_cast<qsizetype>(depth) * 2, QLatin1Char(' '));
            return QStringLiteral("%1%2\u2500\u2500 %3").arg(indent, tw, label);
        }

        QString text(static_cast<qsizetype>(depth) * 2, QLatin1Char(' '));
        if (srcData(IntegrationsTreeModel::HasChildrenRole).toBool()) {
            text += srcData(IntegrationsTreeModel::ExpandedRole).toBool()
                        ? QStringLiteral("\u25be ")
                        : QStringLiteral("\u25b8 ");
        }
        text += label;
        const int count = srcData(IntegrationsTreeModel::CountRole).toInt();
        if (count >= 0) {
            text += QStringLiteral("  (%1)").arg(count);
        }
        return text;
    }

    // A connection/presence dot on account roots (proving the model's connection role maps onto a
    // Tui list decoration), coloured by the coarse connection token.
    const QString rowKind = srcData(IntegrationsTreeModel::RowKindRole).toString();
    if (role == Tui::LeftDecorationRole) {
        if (rowKind == QStringLiteral("account") &&
            !srcData(IntegrationsTreeModel::ConnectionRole).toString().isEmpty()) {
            return QStringLiteral("\u25cf");
        }
        return {};
    }
    if (role == Tui::LeftDecorationFgRole) {
        if (rowKind == QStringLiteral("account")) {
            const QString conn = srcData(IntegrationsTreeModel::ConnectionRole).toString();
            const bool up = conn == QStringLiteral("ready") || conn == QStringLiteral("connected");
            return up ? QVariant::fromValue(tpal::statusOk()) : QVariant::fromValue(tpal::muted());
        }
        return {};
    }
    if (role == Tui::LeftDecorationSpaceRole) {
        return 1;
    }

    return sourceModel()->data(src, role);
}

QVariant DisplayRoleAdapter::listData(const QModelIndex& src, int role) const {
    const auto srcData = [&](int r) { return sourceModel()->data(src, r); };

    if (role == Qt::DisplayRole) {
        // (The owning-agent suffix died with the permanently-empty unit lookup — AD 1a.4.)
        return srcData(SessionsListModel::TitleRole).toString();
    }

    return sourceModel()->data(src, role);
}
