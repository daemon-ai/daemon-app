// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <array>
#include <QStringView>

namespace daemonapp::daemon {

enum class IntegrationStep {
    ConnectUnixSocket,
    Health,
    SessionsQuery,
    Submit,
    Subscribe,
    PersistCursor,
    RespondApproval,
};

inline constexpr std::array<IntegrationStep, 7> kFirstIntegrationSlice = {
    IntegrationStep::ConnectUnixSocket, IntegrationStep::Health,
    IntegrationStep::SessionsQuery,     IntegrationStep::Submit,
    IntegrationStep::Subscribe,         IntegrationStep::PersistCursor,
    IntegrationStep::RespondApproval,
};

inline QStringView integrationStepName(IntegrationStep step) {
    switch (step) {
    case IntegrationStep::ConnectUnixSocket:
        return QStringView(u"connect-unix-socket");
    case IntegrationStep::Health:
        return QStringView(u"health");
    case IntegrationStep::SessionsQuery:
        return QStringView(u"sessions-query");
    case IntegrationStep::Submit:
        return QStringView(u"submit");
    case IntegrationStep::Subscribe:
        return QStringView(u"subscribe");
    case IntegrationStep::PersistCursor:
        return QStringView(u"persist-cursor");
    case IntegrationStep::RespondApproval:
        return QStringView(u"respond-approval");
    }
    return QStringView(u"unknown");
}

} // namespace daemonapp::daemon
