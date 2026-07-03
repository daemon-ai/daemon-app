// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Settings page projection (TabModel::Settings). Split out of tui_page_hub.cpp
// so the settings-editing workstream owns one focused TU; the dispatch stays in
// TuiPageHub::pageMarkdownForKind.

#include "config/idaemon_config.h"
#include "connection/iconnection_service.h"
#include "settings/isettings_store.h"
#include "tui_page_hub.h"

QString TuiPageHub::buildSettingsMarkdown() const {
    const auto cfg = [this](const char* key) {
        return m_deps.daemonConfig->value(QString::fromLatin1(key)).toString();
    };
    const auto onoff = [this](const char* key) {
        return m_deps.daemonConfig->value(QString::fromLatin1(key)).toBool() ? tr("on") : tr("off");
    };

    QString md;
    md += tr("# Settings\n\n");
    md += tr("The settings page, shared with the GUI. Edit values in the GUI; "
             "the TUI reflects the same daemon-config + app prefs. **F8** cycles "
             "the theme.\n\n");

    md += tr("## Connection\n\n");
    md += tr("- State: **%1**\n").arg(m_deps.connection->state());
    md += tr("- Mode: %1\n").arg(m_deps.connection->mode());
    md += tr("- Target: `%1`\n").arg(m_deps.connection->target());
    if (m_deps.settings != nullptr) {
        md += tr("- Start a local daemon automatically: %1\n")
                  .arg(m_deps.settings->managedLocalDaemon() ? tr("on") : tr("off"));
        md += tr("- Stop the managed daemon on exit: %1\n")
                  .arg(m_deps.settings->managedDaemonShutdownOnExit() ? tr("on") : tr("off"));
    }
    md += QStringLiteral("\n");

    md += tr("## Model\n\n");
    md += tr("- Default: `%1`\n").arg(cfg("model/default"));
    md += tr("- Reasoning effort: %1\n").arg(cfg("model/effort"));
    md += tr("- Fast mode: %1\n\n").arg(onoff("model/fast"));

    md += tr("## Chat\n\n");
    md += tr("- Stream responses: %1\n").arg(onoff("chat/streaming"));
    md += tr("- Send on Enter: %1\n").arg(onoff("chat/sendOnEnter"));
    md += tr("- Show token counts: %1\n\n").arg(onoff("chat/showTokenCounts"));

    md += tr("## Safety\n\n");
    md += tr("- Approval policy: %1\n").arg(cfg("safety/approvalPolicy"));
    md += tr("- Filesystem access: %1\n").arg(cfg("safety/sandbox"));
    md += tr("- Allow network: %1\n\n").arg(onoff("safety/allowNetwork"));

    md += tr("## Memory & Context\n\n");
    md += tr("- Max context tokens: %1\n")
              .arg(m_deps.daemonConfig->value(QStringLiteral("memory/contextWindow")).toInt());
    md += tr("- Auto-compact: %1\n").arg(onoff("memory/autoCompact"));
    md += tr("- Persist memory: %1\n\n").arg(onoff("memory/persistMemory"));

    md += tr("## Workspace\n\n");
    md += tr("- Root: `%1`\n").arg(cfg("workspace/root"));
    md += tr("- Respect .gitignore: %1\n\n").arg(onoff("workspace/followGitignore"));

    md += tr("## Voice\n\n");
    md += tr("- Enabled: %1\n").arg(onoff("voice/enabled"));
    md += tr("- Transcription model: %1\n\n").arg(cfg("voice/model"));

    md += tr("## Advanced\n\n");
    md += tr("- Log level: %1\n").arg(cfg("advanced/logLevel"));
    md += tr("- Telemetry: %1\n").arg(onoff("advanced/telemetry"));
    md += tr("- Experimental tools: %1\n").arg(onoff("advanced/experimentalTools"));

    return md;
}
