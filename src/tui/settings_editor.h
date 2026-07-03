// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "theme/theme_palette.h"

#include <functional>
#include <QObject>
#include <QVariantMap>
#include <Tui/ZEvent.h>
#include <Tui/ZWidget.h>

class PaletteDialog;
class TuiPageHub;

// Enter/Space editing for the Settings page rows that a pure seam write cannot
// cover: choice rows open a filterable list pick (PaletteDialog), text/numeric
// rows a TextPromptDialog, and the theme / language / zen rows additionally
// re-apply live through the RootWidget hooks (the same behavior the GUI has -
// theme and language apply immediately). In-place toggles never reach this
// class: TuiPageHub::handlePageActionKey flips them directly.
//
// Owned by RootWidget (which passes itself as the dialog parent); kept free of
// RootWidget types so an offscreen test can drive it against fake seams.
class TuiSettingsEditor : public QObject {
    Q_OBJECT

public:
    struct Hooks {
        // Live re-theme (repaint + persist) - RootWidget::applyTheme.
        std::function<void(theme::ThemeName)> applyTheme;
        // Live zen toggle (hide chrome + persist) - RootWidget::toggleDistractionFree.
        std::function<void()> toggleZen;
        // Re-render the active page after a commit - RootWidget::refreshActivePage.
        std::function<void()> refresh;
        // Return keyboard focus to the page after a dialog commit.
        std::function<void()> restoreFocus;
    };

    TuiSettingsEditor(TuiPageHub* hub, Tui::ZWidget* dialogParent, Hooks hooks,
                      QObject* parent = nullptr);

    // Handle Enter/Space on the selected Settings row (called from RootWidget's
    // page-action chain after the hub declined the key). Returns true iff the
    // key was consumed (a dialog opened or the zen toggle flipped).
    bool handleKey(Tui::ZKeyEvent* event);

private:
    void openChoicePicker(const QVariantMap& row);
    void openTextPrompt(const QVariantMap& row) const;
    void commitChoice(const QVariantMap& row, const QString& choiceId) const;
    void commitText(const QVariantMap& row, const QString& text) const;
    void applied() const;

    TuiPageHub* m_hub = nullptr;
    Tui::ZWidget* m_parent = nullptr;
    Hooks m_hooks;
    PaletteDialog* m_picker = nullptr; // lazily created, reused per open
};
