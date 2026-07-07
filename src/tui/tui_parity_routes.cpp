// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "tui_parity_routes.h"

#include "tab_model.h"

namespace tuiroutes {

QSet<int> routedKinds() {
    return {
        TabModel::Transcript, // per-session chat tab (root_widget_tabs.cpp)
        TabModel::File,       // code editor tab (root_widget_tabs.cpp)
        // Manager-hub / page tabs (TuiPageHub::pageMarkdownForKind).
        TabModel::Settings,
        TabModel::Models,
        TabModel::Accounts,
        TabModel::Profiles,
        TabModel::Fleet,
        TabModel::Sessions,
        TabModel::Dashboard,
        TabModel::Approvals,
        TabModel::Routing,
        TabModel::Cron,
        TabModel::Memory,
        TabModel::Profile,
        TabModel::UsersAccess,
        TabModel::Channels,
    };
}

QSet<QString> handledCommands() {
    return {
        // Manager pages (TuiPageHub::openManagerPage, reachable from both the
        // slash funnel and the palette).
        QStringLiteral("settings"),
        QStringLiteral("dashboard"),
        QStringLiteral("models"),
        QStringLiteral("accounts"),
        QStringLiteral("profiles"),
        QStringLiteral("fleet"),
        QStringLiteral("sessions"),
        QStringLiteral("approvals"),
        QStringLiteral("routing"),
        QStringLiteral("cron"),
        // Capability-gated (auth6): routes to the Users & Access page only when
        // the principal holds access_admin (TuiPageHub::openManagerPage).
        QStringLiteral("access"),
        // Window/session actions (RootWidget::handleComposerCommand).
        QStringLiteral("new"),
        QStringLiteral("model"),
        QStringLiteral("help"),
        QStringLiteral("theme"),
        QStringLiteral("distraction"),
        QStringLiteral("reasoning"),
        QStringLiteral("fast"),
        QStringLiteral("verbose"),
        QStringLiteral("find"),
        // App feedback flow (RootWidget::openAppFeedbackPrompt).
        QStringLiteral("feedback"),
        QStringLiteral("title"),
        QStringLiteral("save"),
        QStringLiteral("clear"),
        QStringLiteral("retry"),
        QStringLiteral("edit"),
        QStringLiteral("undo"),
        // Palette-only routes (TuiOverlayHost::openCommandPalette).
        QStringLiteral("search"),
        QStringLiteral("files"),
        // NOT claimed as handled despite explicit handleComposerCommand
        // branches - exempted with reasons in tui_parity_tests.cpp:
        // "usage" (deliberate no-op: live in the footer in both shells) and
        // "compress" (simulated placeholder in both frontends).
    };
}

} // namespace tuiroutes
