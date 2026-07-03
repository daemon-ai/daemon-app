// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// TuiPageHub dispatch + selection/action handling. The per-page markdown
// builders live in per-workstream translation units under pages/ (hub_settings
// / hub_models / hub_accounts / hub_profiles / hub_memory / hub_ops); this TU
// owns the kind -> builder routing and the shared row/selection/key plumbing.

#include "tui_page_hub.h"

#include "accounts/iaccounts_service.h"
#include "automation/icron_store.h"
#include "automation/irouting_store.h"
#include "daemon/principal_model.h"
#include "fleet/iapprovals_inbox.h"
#include "fleet/ifleet_tree.h"
#include "fleet/isession_roster.h"
#include "models/imodel_catalog.h"
#include "pages/hub_detail.h"
#include "profiles/iprofile_store.h"
#include "tab_model.h"

#include <Tui/ZEvent.h>

using hubdetail::rowsOfModel;

TuiPageHub::TuiPageHub(Dependencies deps) : m_deps(deps) {}

QString TuiPageHub::pageMarkdownForKind(int kind, const QString& profileRef) const {
    // Parity: a kind added here must also land in tuiroutes::routedKinds()
    // (tui_parity_routes.cpp) - tests/tui/tui_parity_tests.cpp enforces it.
    const int sel = m_pageSel.value(kind, 0);
    switch (kind) {
    case TabModel::Settings:
        return buildSettingsMarkdown();
    case TabModel::Models:
        return buildModelsMarkdown(sel);
    case TabModel::Accounts:
        return buildAccountsMarkdown(sel);
    case TabModel::Profiles:
        return buildProfilesMarkdown(sel);
    case TabModel::Dashboard:
        return buildDashboardMarkdown();
    case TabModel::Fleet:
        return buildFleetMarkdown(sel);
    case TabModel::Sessions:
        return buildSessionsMarkdown(sel);
    case TabModel::Approvals:
        return buildApprovalsMarkdown(sel);
    case TabModel::Routing:
        return buildRoutingMarkdown(sel);
    case TabModel::Cron:
        return buildCronMarkdown(sel);
    case TabModel::Memory:
        return buildMemoryMarkdown();
    case TabModel::Profile:
        return buildProfileMarkdown(profileRef);
    case TabModel::UsersAccess:
        return buildUsersAccessMarkdown();
    default:
        return {};
    }
}

bool TuiPageHub::openManagerPage(const QString& id) const {
    // Parity: an id added here must also land in tuiroutes::handledCommands()
    // (tui_parity_routes.cpp) - tests/tui/tui_parity_tests.cpp enforces it.
    const QHash<QString, QPair<int, QString>> kPageRoutes = {
        {QStringLiteral("settings"), {TabModel::Settings, tr("Settings")}},
        {QStringLiteral("dashboard"), {TabModel::Dashboard, tr("Dashboard")}},
        {QStringLiteral("models"), {TabModel::Models, tr("Models")}},
        {QStringLiteral("accounts"), {TabModel::Accounts, tr("Accounts")}},
        {QStringLiteral("profiles"), {TabModel::Profiles, tr("Profiles")}},
        {QStringLiteral("fleet"), {TabModel::Fleet, tr("Fleet")}},
        {QStringLiteral("sessions"), {TabModel::Sessions, tr("Sessions")}},
        {QStringLiteral("approvals"), {TabModel::Approvals, tr("Approvals")}},
        {QStringLiteral("routing"), {TabModel::Routing, tr("Routing")}},
        {QStringLiteral("cron"), {TabModel::Cron, tr("Scheduled jobs")}},
        {QStringLiteral("access"), {TabModel::UsersAccess, tr("Users & Access")}},
    };
    const auto route = kPageRoutes.constFind(id);
    if (route == kPageRoutes.constEnd()) {
        return false;
    }
    // Capability gate (auth6, GUI parity with Main.qml/Session.qml): the Users &
    // Access admin page never mounts unless the authenticated principal holds
    // access_admin; the command is still consumed (fail-closed no-op). The palette
    // already hides the gated entry; this also covers the raw "/access" funnel.
    // The node enforces access server-side regardless.
    if (route->first == TabModel::UsersAccess &&
        (m_deps.principal == nullptr ||
         !m_deps.principal->hasCapability(QStringLiteral("access_admin")))) {
        return true;
    }
    if (m_deps.tabModel != nullptr) {
        m_deps.tabModel->openPage(route->first, route->second);
    }
    return true;
}

int TuiPageHub::activePageKind(bool transcriptActive) const {
    if (transcriptActive || m_deps.tabModel == nullptr) {
        return -1;
    }
    const int idx = m_deps.tabModel->currentIndex();
    if (idx < 0) {
        return -1;
    }
    switch (m_deps.tabModel->kindAt(idx)) {
    case TabModel::Models:
    case TabModel::Accounts:
    case TabModel::Profiles:
    case TabModel::Sessions:
    case TabModel::Fleet:
    case TabModel::Approvals:
    case TabModel::Routing:
    case TabModel::Cron:
        return m_deps.tabModel->kindAt(idx);
    default:
        return -1;
    }
}

QList<QVariantMap> TuiPageHub::pageActionRows(int kind) const {
    switch (kind) {
    case TabModel::Models:
        return rowsOfModel(m_deps.modelCatalog->installed());
    case TabModel::Accounts:
        return rowsOfModel(m_deps.accounts->accounts());
    case TabModel::Profiles:
        return rowsOfModel(m_deps.profiles->profiles());
    case TabModel::Sessions:
        return rowsOfModel(m_deps.roster->sessions());
    case TabModel::Fleet:
        return rowsOfModel(m_deps.fleetTree->nodes());
    case TabModel::Approvals:
        return rowsOfModel(m_deps.approvals->pending());
    case TabModel::Routing:
        return rowsOfModel(m_deps.routing->rules());
    case TabModel::Cron:
        return rowsOfModel(m_deps.cron->jobs());
    default:
        return {};
    }
}

void TuiPageHub::clampSelection(int kind) {
    const int rows = static_cast<int>(pageActionRows(kind).size());
    if (rows > 0) {
        int& sel = m_pageSel[kind];
        sel = qBound(0, sel, rows - 1);
    }
}

void TuiPageHub::moveSelection(int kind, int delta) {
    const int rows = static_cast<int>(pageActionRows(kind).size());
    if (rows == 0) {
        return;
    }
    int& sel = m_pageSel[kind];
    sel = qBound(0, sel + delta, rows - 1);
}

bool TuiPageHub::handlePageActionKey(int kind, Tui::ZKeyEvent* event) {
    if (event->modifiers() != Qt::NoModifier || kind < 0) {
        return false;
    }
    const QList<QVariantMap> rows = pageActionRows(kind);
    const QString text = event->text();
    const int key = event->key();

    if (key == Qt::Key_Up || text == QStringLiteral("k")) {
        moveSelection(kind, -1);
        event->accept();
        return true;
    }
    if (key == Qt::Key_Down || text == QStringLiteral("j")) {
        moveSelection(kind, 1);
        event->accept();
        return true;
    }

    if (rows.isEmpty()) {
        return false;
    }
    const int sel = qBound(0, m_pageSel.value(kind, 0), static_cast<int>(rows.size()) - 1);
    const QVariantMap& row = rows.at(sel);
    const QString id = row.value(QStringLiteral("id")).toString();
    const bool enter = key == Qt::Key_Enter || key == Qt::Key_Return;
    bool acted = false;

    switch (kind) {
    case TabModel::Models:
        if (enter) {
            m_deps.modelCatalog->activate(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.modelCatalog->remove(id);
            acted = true;
        }
        break;
    case TabModel::Accounts:
        if (enter || text == QStringLiteral("R")) {
            m_deps.accounts->reauth(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.accounts->remove(id);
            acted = true;
        }
        break;
    case TabModel::Profiles:
        if (enter) {
            m_deps.profiles->setDefault(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.profiles->remove(id);
            acted = true;
        } else if (text == QStringLiteral("n")) {
            // New profile via clone of the selected one (a copy; full edit stays in the GUI).
            m_deps.profiles->cloneProfile(id, id + QStringLiteral("-copy"));
            acted = true;
        }
        break;
    case TabModel::Sessions:
        if (text == QStringLiteral("s")) {
            m_deps.roster->suspend(id);
            acted = true;
        } else if (text == QStringLiteral("R") || enter) {
            m_deps.roster->resume(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.roster->close(id);
            acted = true;
        }
        break;
    case TabModel::Fleet:
        if (key == Qt::Key_Space || enter || text == QStringLiteral("p")) {
            const bool paused =
                row.value(QStringLiteral("status")).toString() == QLatin1String("paused");
            if (paused) {
                m_deps.fleetTree->resume(id);
            } else {
                m_deps.fleetTree->pause(id);
            }
            acted = true;
        }
        break;
    case TabModel::Approvals:
        if (text == QStringLiteral("a") || enter) {
            m_deps.approvals->approve(id);
            acted = true;
        } else if (text == QStringLiteral("d")) {
            m_deps.approvals->deny(id);
            acted = true;
        }
        break;
    case TabModel::Routing:
        if (key == Qt::Key_Space || enter) {
            m_deps.routing->setEnabled(id, !row.value(QStringLiteral("enabled")).toBool());
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.routing->remove(id);
            acted = true;
        }
        break;
    case TabModel::Cron:
        if (key == Qt::Key_Space || enter) {
            m_deps.cron->setEnabled(id, !row.value(QStringLiteral("enabled")).toBool());
            acted = true;
        } else if (text == QStringLiteral("t")) {
            m_deps.cron->runNow(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.cron->remove(id);
            acted = true;
        }
        break;
    default:
        break;
    }

    if (acted) {
        event->accept();
        return true;
    }
    return false;
}
