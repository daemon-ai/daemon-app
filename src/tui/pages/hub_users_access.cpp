// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Users & Access admin page projection (TabModel::UsersAccess, auth6): the live
// WhoAmI panel plus the user/role administration scaffold, mirroring the GUI's
// UsersAccessPage.qml. Read-only: user CRUD pends the node access-admin API
// (Auth 5) - the GUI's only action ("Add user") is disabled for the same reason.
// The route is gated on access_admin (see openManagerPage); the node enforces
// access server-side regardless.

#include "daemon/principal_model.h"
#include "tui_page_hub.h"

QString TuiPageHub::buildUsersAccessMarkdown() const {
    const daemonapp::daemon::PrincipalModel* principal = m_deps.principal;
    const bool authenticated = principal != nullptr && principal->authenticated();

    QString md;
    md += tr("# Users & Access\n\n");

    // --- WhoAmI (live, from the AuthOk PrincipalView) ---
    md += tr("## Signed in as\n\n");
    if (authenticated) {
        md += tr("- **%1**\n").arg(principal->username());
        if (!principal->userId().isEmpty()) {
            md += tr("- User id: `%1`\n").arg(principal->userId());
        }
    } else {
        md += tr("- **Not authenticated**\n");
    }
    const QStringList roles = principal != nullptr ? principal->roles() : QStringList();
    const QStringList capabilities =
        principal != nullptr ? principal->capabilities() : QStringList();
    md +=
        tr("- Roles: %1\n").arg(roles.isEmpty() ? tr("(none)") : roles.join(QStringLiteral(", ")));
    md += tr("- Capabilities: %1\n\n")
              .arg(capabilities.isEmpty() ? tr("(none)") : capabilities.join(QStringLiteral(", ")));

    // --- User management (pending Auth 5: access-admin-api) ---
    md += tr("## Users & roles\n\n");
    md += tr("User administration is not available yet.\n\n");
    md += tr("Listing, creating, disabling users and assigning roles requires the "
             "node access-admin API, which is still being built.\n");
    return md;
}
