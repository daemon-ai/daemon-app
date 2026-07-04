// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "display_presenter.h"

#include "domain/sidebar_node.h"
#include "domain/unit_node.h"

DisplayPresenter::DisplayPresenter(QObject* parent) : QObject(parent) {}

QString DisplayPresenter::scopeIconKeyFor(int nodeType) {
    switch (static_cast<domain::NodeType>(nodeType)) {
    case domain::NodeType::AllSessions:
        return QStringLiteral("comments");
    case domain::NodeType::Archived:
        return QStringLiteral("archive");
    default:
        return {};
    }
}

QString DisplayPresenter::agentKindIconKeyFor(int kind) {
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

DisplayPresenter::StateTone DisplayPresenter::agentStateToneFor(int state) {
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

QString DisplayPresenter::enumLabelFor(const QString& family, const QString& code) {
    QString c = code.trimmed();
    if (c.isEmpty()) {
        return {};
    }
    const QString key = c.toLower();

    if (family == QLatin1String("connection")) {
        if (key == QLatin1String("checking"))
            return tr("checking", "connection state");
        if (key == QLatin1String("connecting"))
            return tr("connecting", "connection state");
        if (key == QLatin1String("authenticating"))
            return tr("authenticating", "connection state");
        if (key == QLatin1String("ready"))
            return tr("ready", "connection state");
        if (key == QLatin1String("offline"))
            return tr("offline", "connection state");
        if (key == QLatin1String("needs setup"))
            return tr("needs setup", "connection state");
    } else if (family == QLatin1String("memory.tier")) {
        if (key == QLatin1String("working"))
            return tr("working", "memory tier");
        if (key == QLatin1String("episodic"))
            return tr("episodic", "memory tier");
        if (key == QLatin1String("scratchpad"))
            return tr("scratchpad", "memory tier");
    } else if (family == QLatin1String("memory.veracity")) {
        if (key == QLatin1String("stated"))
            return tr("stated", "memory veracity");
        if (key == QLatin1String("inferred"))
            return tr("inferred", "memory veracity");
        if (key == QLatin1String("tool"))
            return tr("tool", "memory veracity");
        if (key == QLatin1String("imported"))
            return tr("imported", "memory veracity");
        if (key == QLatin1String("unknown"))
            return tr("unknown", "memory veracity");
    } else if (family == QLatin1String("memory.status")) {
        if (key == QLatin1String("active"))
            return tr("active", "memory status");
        if (key == QLatin1String("expired"))
            return tr("expired", "memory status");
        if (key == QLatin1String("superseded"))
            return tr("superseded", "memory status");
    } else if (family == QLatin1String("memory.degradation")) {
        if (key == QLatin1String("hot"))
            return tr("hot", "memory degradation");
        if (key == QLatin1String("warm"))
            return tr("warm", "memory degradation");
        if (key == QLatin1String("cold"))
            return tr("cold", "memory degradation");
    } else if (family == QLatin1String("memory.nodeKind")) {
        if (key == QLatin1String("memory"))
            return tr("memory", "memory node kind");
        if (key == QLatin1String("entity"))
            return tr("entity", "memory node kind");
        if (key == QLatin1String("fact"))
            return tr("fact", "memory node kind");
        if (key == QLatin1String("gist"))
            return tr("gist", "memory node kind");
        if (key == QLatin1String("conflict"))
            return tr("conflict", "memory node kind");
    } else if (family == QLatin1String("memory.eventKind")) {
        if (key == QLatin1String("added"))
            return tr("added", "memory event kind");
        if (key == QLatin1String("updated"))
            return tr("updated", "memory event kind");
        if (key == QLatin1String("recalled"))
            return tr("recalled", "memory event kind");
        if (key == QLatin1String("invalidated"))
            return tr("invalidated", "memory event kind");
        if (key == QLatin1String("consolidated"))
            return tr("consolidated", "memory event kind");
        if (key == QLatin1String("snapshot"))
            return tr("snapshot", "memory event kind");
    } else if (family == QLatin1String("approval.risk")) {
        if (key == QLatin1String("high"))
            return tr("high", "approval risk");
        if (key == QLatin1String("medium"))
            return tr("medium", "approval risk");
        if (key == QLatin1String("low"))
            return tr("low", "approval risk");
    } else if (family == QLatin1String("download.state")) {
        if (key == QLatin1String("queued"))
            return tr("queued", "download state");
        if (key == QLatin1String("paused"))
            return tr("paused", "download state");
        if (key == QLatin1String("downloading"))
            return tr("downloading", "download state");
        if (key == QLatin1String("done"))
            return tr("done", "download state");
        if (key == QLatin1String("failed"))
            return tr("failed", "download state");
        if (key == QLatin1String("cancelled"))
            return tr("cancelled", "download state");
    }

    // Unknown family or code: render the raw token so new node vocabulary is not
    // silently dropped.
    return c;
}

QString DisplayPresenter::enumLabel(const QString& family, const QString& code) const {
    return enumLabelFor(family, code);
}

QString DisplayPresenter::scopeIconKey(int nodeType) const {
    return scopeIconKeyFor(nodeType);
}

QString DisplayPresenter::agentKindIconKey(int kind) const {
    return agentKindIconKeyFor(kind);
}

DisplayPresenter::StateTone DisplayPresenter::agentStateTone(int state) const {
    return agentStateToneFor(state);
}
