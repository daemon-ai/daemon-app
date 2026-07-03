// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "settings_editor.h"

#include "i18n/localization.h"
#include "palette_dialog.h"
#include "tab_model.h"
#include "tui_dialogs.h"
#include "tui_page_hub.h"

#include <QVector>

TuiSettingsEditor::TuiSettingsEditor(TuiPageHub* hub, Tui::ZWidget* dialogParent, Hooks hooks,
                                     QObject* parent)
    : QObject(parent), m_hub(hub), m_parent(dialogParent), m_hooks(std::move(hooks)) {}

bool TuiSettingsEditor::handleKey(Tui::ZKeyEvent* event) {
    if (m_hub == nullptr || event->modifiers() != Qt::NoModifier) {
        return false;
    }
    const int key = event->key();
    const bool enter = key == Qt::Key_Enter || key == Qt::Key_Return;
    if (!enter && key != Qt::Key_Space) {
        return false;
    }
    const QList<QVariantMap> rows = m_hub->pageActionRows(TabModel::Settings);
    if (rows.isEmpty()) {
        return false;
    }
    const int sel =
        qBound(0, m_hub->pageSelection(TabModel::Settings), static_cast<int>(rows.size()) - 1);
    const QVariantMap& row = rows.at(sel);
    const QString type = row.value(QStringLiteral("type")).toString();

    // Zen is a toggle, but its apply is the live RootWidget path (which also
    // persists), so it lands here rather than in the hub's in-place flip.
    if (row.value(QStringLiteral("seam")).toString() == QLatin1String("zen")) {
        if (m_hooks.toggleZen) {
            m_hooks.toggleZen();
        }
        applied();
        event->accept();
        return true;
    }
    if (type == QLatin1String("choice")) {
        openChoicePicker(row);
        event->accept();
        return true;
    }
    if (type == QLatin1String("text") && enter) {
        openTextPrompt(row);
        event->accept();
        return true;
    }
    return false;
}

void TuiSettingsEditor::openChoicePicker(const QVariantMap& row) {
    if (m_picker == nullptr) {
        m_picker = new PaletteDialog(QString(), m_parent);
    }
    m_picker->setWindowTitle(row.value(QStringLiteral("label")).toString());
    // Rewire each open to capture the current row (cheap, avoids stale lambdas -
    // the TuiOverlayHost pickers do the same).
    disconnect(m_picker, &PaletteDialog::activated, this, nullptr);
    connect(m_picker, &PaletteDialog::activated, this,
            [this, row](const QString& choiceId) { commitChoice(row, choiceId); });

    const QStringList ids = row.value(QStringLiteral("options")).toStringList();
    const QStringList labels = row.value(QStringLiteral("optionLabels")).toStringList();
    const QString current = row.value(QStringLiteral("value")).toString();
    QVector<PaletteDialog::Item> items;
    items.reserve(ids.size());
    for (int i = 0; i < ids.size(); ++i) {
        const QString label = i < labels.size() ? labels.at(i) : ids.at(i);
        items.push_back(
            {ids.at(i),
             (ids.at(i) == current ? QStringLiteral("✓ ") : QStringLiteral("  ")) + label,
             QString()});
    }
    m_picker->setItems(items);
    m_picker->openCentered();
}

void TuiSettingsEditor::openTextPrompt(const QVariantMap& row) const {
    auto* dialog = new TextPromptDialog(row.value(QStringLiteral("label")).toString(),
                                        row.value(QStringLiteral("value")).toString(),
                                        /*masked=*/false, m_parent);
    connect(dialog, &TextPromptDialog::submitted, this,
            [this, row](const QString& text) { commitText(row, text); });
    connect(dialog, &TextPromptDialog::canceled, this, [this] {
        if (m_hooks.restoreFocus) {
            m_hooks.restoreFocus();
        }
    });
}

void TuiSettingsEditor::commitChoice(const QVariantMap& row, const QString& choiceId) const {
    const QString seam = row.value(QStringLiteral("seam")).toString();
    if (seam == QLatin1String("theme")) {
        // Live re-theme through the F8 machinery (persists the shared ui/theme).
        if (m_hooks.applyTheme) {
            m_hooks.applyTheme(theme::ThemePalette::fromString(choiceId));
        }
    } else {
        m_hub->applySettingsValue(row, choiceId);
        if (seam == QLatin1String("language")) {
            // Same live path as the GUI's languageChanged -> applyLocale; freshly
            // built strings (this page, dialogs) translate now, the chrome built
            // at startup follows on restart (noted on the row).
            i18n::applyLocale(choiceId);
        }
    }
    applied();
}

void TuiSettingsEditor::commitText(const QVariantMap& row, const QString& text) const {
    // Numeric rows mirror the GUI's parseInt(text) || 0 (toInt is 0 on failure).
    m_hub->applySettingsValue(row, row.value(QStringLiteral("numeric")).toBool()
                                       ? QVariant(text.toInt())
                                       : QVariant(text));
    applied();
}

void TuiSettingsEditor::applied() const {
    if (m_hooks.refresh) {
        m_hooks.refresh();
    }
    if (m_hooks.restoreFocus) {
        m_hooks.restoreFocus();
    }
}
