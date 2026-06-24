#include "display_presenter.h"

#include "domain/unit_node.h"
#include "domain/sidebar_node.h"

DisplayPresenter::DisplayPresenter(QObject* parent) : QObject(parent) {}

QString DisplayPresenter::scopeIconKeyFor(int nodeType)
{
    switch (static_cast<domain::NodeType>(nodeType)) {
    case domain::NodeType::AllConversations:
        return QStringLiteral("comments");
    case domain::NodeType::Archived:
        return QStringLiteral("archive");
    default:
        return {};
    }
}

QString DisplayPresenter::agentKindIconKeyFor(int kind)
{
    switch (static_cast<domain::UnitKind>(kind)) {
    case domain::UnitKind::Orchestrator:
        return QStringLiteral("sitemap");
    case domain::UnitKind::Host:
        return QStringLiteral("server");
    case domain::UnitKind::Engine:
    default:
        return QStringLiteral("robot");
    }
}

DisplayPresenter::StateTone DisplayPresenter::agentStateToneFor(int state)
{
    switch (static_cast<domain::UnitState>(state)) {
    case domain::UnitState::Running:
        return StateTone::Running;
    case domain::UnitState::Finished:
        return StateTone::Finished;
    case domain::UnitState::Unknown:
    default:
        return StateTone::Neutral;
    }
}

QString DisplayPresenter::scopeIconKey(int nodeType) const
{
    return scopeIconKeyFor(nodeType);
}

QString DisplayPresenter::agentKindIconKey(int kind) const
{
    return agentKindIconKeyFor(kind);
}

DisplayPresenter::StateTone DisplayPresenter::agentStateTone(int state) const
{
    return agentStateToneFor(state);
}
