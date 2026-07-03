// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Settings page projection + row actions (TabModel::Settings). Split out of
// tui_page_hub.cpp so the settings-editing workstream owns one focused TU; the
// dispatch stays in TuiPageHub::pageMarkdownForKind / handlePageActionKey.
//
// The page mirrors the GUI settings sections that make REAL seam-backed writes
// (see src/DaemonApp/Pages/*Section.qml): every editable row persists through
// the exact same call the GUI makes - ISettingsStore::setValue for app prefs,
// IDaemonConfig::setValue for node config. Toggles flip in place here; rows that
// need a dialog or a live re-apply (theme / language pickers, text prompts, the
// zen toggle) decline the key so RootWidget's TuiSettingsEditor handles them.

#include "config/idaemon_config.h"
#include "connection/iconnection_service.h"
#include "i18n/localization.h"
#include "models/imodel_catalog.h"
#include "settings/isettings_store.h"
#include "theme/theme_palette.h"
#include "tui_page_hub.h"
#include "tui_palette.h"

#include <QSettings>

namespace {

// Row-map vocabulary shared with TuiSettingsEditor (settings_editor.cpp):
//   id      - the storage key (unique row id)
//   type    - "toggle" | "choice" | "text"
//   seam    - "config" (IDaemonConfig) | "settings" (ISettingsStore) |
//             "theme" / "language" / "zen" (RootWidget-applied live paths)
//   value   - current value (bool / string / int)
//   options / optionLabels - choice ids + display labels (choice rows)
//   note    - trailing italic remark rendered after the value
//   numeric - text row committing an int (GUI parity: parseInt(text) || 0)
QVariantMap makeRow(const QString& section, const QString& sectionLabel, const QString& id,
                    const QString& label, const QString& type, const QString& seam,
                    const QVariant& value) {
    return {
        {QStringLiteral("section"), section}, {QStringLiteral("sectionLabel"), sectionLabel},
        {QStringLiteral("id"), id},           {QStringLiteral("label"), label},
        {QStringLiteral("type"), type},       {QStringLiteral("seam"), seam},
        {QStringLiteral("value"), value},
    };
}

QString rowValueText(const QVariantMap& row) {
    const QString type = row.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("toggle")) {
        return row.value(QStringLiteral("value")).toBool() ? TuiPageHub::tr("on")
                                                           : TuiPageHub::tr("off");
    }
    if (type == QLatin1String("choice")) {
        const QString current = row.value(QStringLiteral("value")).toString();
        const QStringList ids = row.value(QStringLiteral("options")).toStringList();
        const QStringList labels = row.value(QStringLiteral("optionLabels")).toStringList();
        const int idx = static_cast<int>(ids.indexOf(current));
        const QString shown = (idx >= 0 && idx < labels.size()) ? labels.at(idx) : current;
        return QStringLiteral("**") + shown + QStringLiteral("**");
    }
    const QString current = row.value(QStringLiteral("value")).toString();
    if (current.isEmpty()) {
        return TuiPageHub::tr("_(not set)_");
    }
    return QStringLiteral("`") + current + QStringLiteral("`");
}

} // namespace

QList<QVariantMap> TuiPageHub::settingsActionRows() const {
    QList<QVariantMap> rows;

    // Appearance - mirrors AppearanceSection.qml's theme swatches + language
    // picker (UiSettings) and the distraction-free layout toggle. The TUI applies
    // theme + zen live itself; the shared "ui/*" keys keep the GUI in sync.
    const QString appearance = QStringLiteral("appearance");
    const QString appearanceLabel = tr("Appearance");
    {
        QVariantMap theme = makeRow(appearance, appearanceLabel, QStringLiteral("ui/theme"),
                                    tr("Theme"), QStringLiteral("choice"), QStringLiteral("theme"),
                                    theme::ThemePalette::toString(tpal::activeTheme()));
        theme.insert(QStringLiteral("options"), theme::ThemePalette::allNames());
        theme.insert(QStringLiteral("optionLabels"), theme::ThemePalette::allNames());
        rows << theme;
    }
    if (m_deps.settings != nullptr) {
        QStringList codes;
        QStringList labels;
        for (const i18n::LocaleOption& option : i18n::availableLocales()) {
            codes << option.code;
            labels << option.label;
        }
        QVariantMap language =
            makeRow(appearance, appearanceLabel, QStringLiteral("ui/language"), tr("Language"),
                    QStringLiteral("choice"), QStringLiteral("language"),
                    m_deps.settings->value(QStringLiteral("ui/language"), QStringLiteral("system"))
                        .toString());
        language.insert(QStringLiteral("options"), codes);
        language.insert(QStringLiteral("optionLabels"), labels);
        language.insert(QStringLiteral("note"),
                        tr("applies live; restart to retranslate the whole chrome"));
        rows << language;
    }
    // Zen persists per-frontend (the TUI's own QSettings scope, like Wave 0's
    // /distraction), so read the same key setDistractionFree writes.
    rows << makeRow(appearance, appearanceLabel, QStringLiteral("ui/distractionFree"),
                    tr("Distraction-free (hide chrome)"), QStringLiteral("toggle"),
                    QStringLiteral("zen"),
                    QSettings().value(QStringLiteral("ui/distractionFree"), false).toBool());

    // Notifications - NotificationsSection.qml (AppSettings/ISettingsStore).
    if (m_deps.settings != nullptr) {
        const QString section = QStringLiteral("notifications");
        const QString label = tr("Notifications");
        rows << makeRow(section, label, QStringLiteral("notify/gates"),
                        tr("Notify when a turn needs my input"), QStringLiteral("toggle"),
                        QStringLiteral("settings"),
                        m_deps.settings->value(QStringLiteral("notify/gates"), true).toBool());
        rows << makeRow(section, label, QStringLiteral("notify/turnDone"),
                        tr("Notify when a turn finishes"), QStringLiteral("toggle"),
                        QStringLiteral("settings"),
                        m_deps.settings->value(QStringLiteral("notify/turnDone"), false).toBool());
    }

    // Connection - ConnectionSection.qml's managed-local-daemon prefs
    // (ISettingsStore); the live state/target stay read-only context lines.
    if (m_deps.settings != nullptr) {
        const QString section = QStringLiteral("connection");
        const QString label = tr("Connection");
        rows << makeRow(section, label, QStringLiteral("conn/managedLocalDaemon"),
                        tr("Start a local daemon automatically"), QStringLiteral("toggle"),
                        QStringLiteral("settings"), m_deps.settings->managedLocalDaemon());
        rows << makeRow(section, label, QStringLiteral("conn/managedDaemonShutdownOnExit"),
                        tr("Stop the managed daemon when I close the app"),
                        QStringLiteral("toggle"), QStringLiteral("settings"),
                        m_deps.settings->managedDaemonShutdownOnExit());
    }

    if (m_deps.daemonConfig == nullptr) {
        return rows;
    }
    const auto configToggle = [this](const QString& section, const QString& sectionLabel,
                                     const char* key, const QString& label, bool fallback) {
        const QString id = QString::fromLatin1(key);
        return makeRow(section, sectionLabel, id, label, QStringLiteral("toggle"),
                       QStringLiteral("config"), m_deps.daemonConfig->value(id, fallback).toBool());
    };
    const auto configChoice = [this](const QString& section, const QString& sectionLabel,
                                     const char* key, const QString& label) {
        const QString id = QString::fromLatin1(key);
        QVariantMap row =
            makeRow(section, sectionLabel, id, label, QStringLiteral("choice"),
                    QStringLiteral("config"), m_deps.daemonConfig->value(id).toString());
        row.insert(QStringLiteral("options"), m_deps.daemonConfig->options(id));
        row.insert(QStringLiteral("optionLabels"), m_deps.daemonConfig->options(id));
        return row;
    };
    const auto configText = [this](const QString& section, const QString& sectionLabel,
                                   const char* key, const QString& label, bool numeric) {
        const QString id = QString::fromLatin1(key);
        QVariantMap row = makeRow(section, sectionLabel, id, label, QStringLiteral("text"),
                                  QStringLiteral("config"), m_deps.daemonConfig->value(id));
        if (numeric) {
            row.insert(QStringLiteral("numeric"), true);
        }
        return row;
    };

    // Model - ModelSettingsSection.qml's inference knobs (IDaemonConfig). The
    // default model itself is read-only context (picked in the Models hub).
    const QString model = QStringLiteral("model");
    const QString modelLabel = tr("Model");
    rows << configChoice(model, modelLabel, "model/effort", tr("Reasoning effort"));
    rows << configToggle(model, modelLabel, "model/fast", tr("Fast mode (lower latency)"), false);

    // Chat - ChatSettingsSection.qml (IDaemonConfig).
    const QString chat = QStringLiteral("chat");
    const QString chatLabel = tr("Chat");
    rows << configToggle(chat, chatLabel, "chat/streaming", tr("Stream responses"), true);
    rows << configToggle(chat, chatLabel, "chat/sendOnEnter",
                         tr("Send on Enter (Shift+Enter for newline)"), true);
    rows << configToggle(chat, chatLabel, "chat/showTokenCounts", tr("Show token counts"), true);
    rows << configText(chat, chatLabel, "chat/systemPrompt",
                       tr("Default system prompt for new chats"), false);

    // Safety - SafetySettingsSection.qml (IDaemonConfig).
    const QString safety = QStringLiteral("safety");
    const QString safetyLabel = tr("Safety");
    rows << configChoice(safety, safetyLabel, "safety/approvalPolicy", tr("Approval policy"));
    rows << configChoice(safety, safetyLabel, "safety/sandbox", tr("Filesystem access"));
    rows << configToggle(safety, safetyLabel, "safety/allowNetwork", tr("Allow network access"),
                         false);

    // Memory & Context - MemorySettingsSection.qml (IDaemonConfig).
    const QString memory = QStringLiteral("memory");
    const QString memoryLabel = tr("Memory & Context");
    rows << configText(memory, memoryLabel, "memory/contextWindow", tr("Max context tokens"), true);
    rows << configToggle(memory, memoryLabel, "memory/autoCompact",
                         tr("Auto-compact when context is full"), true);
    rows << configToggle(memory, memoryLabel, "memory/persistMemory",
                         tr("Persist memory across sessions"), true);

    // Workspace - WorkspaceSection.qml (IDaemonConfig).
    const QString workspace = QStringLiteral("workspace");
    const QString workspaceLabel = tr("Workspace");
    rows << configText(workspace, workspaceLabel, "workspace/root", tr("Workspace root"), false);
    rows << configToggle(workspace, workspaceLabel, "workspace/followGitignore",
                         tr("Respect .gitignore"), true);

    // Voice - VoiceSection.qml (IDaemonConfig).
    const QString voice = QStringLiteral("voice");
    const QString voiceLabel = tr("Voice");
    rows << configToggle(voice, voiceLabel, "voice/enabled", tr("Enable voice transcription"),
                         false);
    rows << configChoice(voice, voiceLabel, "voice/model", tr("Transcription model"));

    // Advanced - AdvancedSection.qml's diagnostics rows (IDaemonConfig). The
    // "re-run first-run setup" action stays GUI/first-run-workstream territory.
    const QString advanced = QStringLiteral("advanced");
    const QString advancedLabel = tr("Advanced");
    rows << configChoice(advanced, advancedLabel, "advanced/logLevel", tr("Log level"));
    rows << configToggle(advanced, advancedLabel, "advanced/telemetry",
                         tr("Send anonymous telemetry"), false);
    rows << configToggle(advanced, advancedLabel, "advanced/experimentalTools",
                         tr("Enable experimental tools"), false);

    return rows;
}

bool TuiPageHub::applySettingsValue(const QVariantMap& row, const QVariant& value) const {
    const QString seam = row.value(QStringLiteral("seam")).toString();
    const QString key = row.value(QStringLiteral("id")).toString();
    if (seam == QLatin1String("config") && m_deps.daemonConfig != nullptr) {
        m_deps.daemonConfig->setValue(key, value);
        return true;
    }
    // "language" persists like any app pref (the shared ui/language key); the
    // live locale re-apply is the editor's job. "theme"/"zen" have no seam write
    // here at all: RootWidget's applyTheme / setDistractionFree own persistence.
    const bool settingsBacked =
        seam == QLatin1String("settings") || seam == QLatin1String("language");
    if (settingsBacked && m_deps.settings != nullptr) {
        m_deps.settings->setValue(key, value);
        return true;
    }
    return false;
}

bool TuiPageHub::applySettingsToggle(const QVariantMap& row, bool activate) const {
    if (!activate || row.value(QStringLiteral("type")).toString() != QLatin1String("toggle")) {
        return false;
    }
    return applySettingsValue(row, !row.value(QStringLiteral("value")).toBool());
}

QString TuiPageHub::buildSettingsMarkdown(int sel) const {
    const QList<QVariantMap> rows = settingsActionRows();
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Settings\n\n");
    md += tr("Shared with the GUI — both front ends edit the same daemon config "
             "and app prefs. **j/k** select a row · **Space/Enter** toggles · "
             "**Enter** opens a picker/editor · **F8** cycles the theme.\n");

    QString section;
    for (int i = 0; i < rows.size(); ++i) {
        const QVariantMap& row = rows.at(i);
        if (row.value(QStringLiteral("section")).toString() != section) {
            section = row.value(QStringLiteral("section")).toString();
            md += QStringLiteral("\n## ") + row.value(QStringLiteral("sectionLabel")).toString() +
                  QStringLiteral("\n\n");
            // Read-only context lines the GUI section shows above its controls.
            if (section == QLatin1String("connection") && m_deps.connection != nullptr) {
                md += tr("- State: **%1** · %2 · `%3` — _reconnect via the GUI picker or the "
                         "first-run gate_\n")
                          .arg(m_deps.connection->state(), m_deps.connection->mode(),
                               m_deps.connection->target());
            } else if (section == QLatin1String("model") && m_deps.modelCatalog != nullptr) {
                const QString current = m_deps.modelCatalog->currentModelId();
                md += tr("- Active default: `%1` — _change or install in the Models page "
                         "(`/models`)_\n")
                          .arg(current.isEmpty() ? QStringLiteral("-") : current);
            }
        }
        md += QStringLiteral("- ") + mark(i) + row.value(QStringLiteral("label")).toString() +
              QStringLiteral(": ") + rowValueText(row);
        const QString note = row.value(QStringLiteral("note")).toString();
        if (!note.isEmpty()) {
            md += QStringLiteral(" — _") + note + QStringLiteral("_");
        }
        md += QLatin1Char('\n');
    }
    return md;
}
