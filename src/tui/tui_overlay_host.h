// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "palette_dialog.h"

#include <functional>
#include <QObject>
#include <Tui/ZWidget.h>

class CommandRegistry;
class ComposerSessionController;
namespace models {
class IModelCatalog;
}

class TuiOverlayHost : public QObject {
    Q_OBJECT

public:
    struct CommandCallbacks {
        std::function<bool(const QString&)> openManagerPage;
        std::function<void()> newTranscriptTab;
        std::function<void()> cycleTheme;
        std::function<void()> openModelPicker;
        std::function<void()> focusSearch;
        std::function<void()> toggleExplorer;
        std::function<void()> openTranscriptSearch;
        std::function<void(const QString&)> invokeActiveCommand;
        std::function<void()> focusComposer;
    };

    explicit TuiOverlayHost(Tui::ZWidget* parent);

    void promptQuit(const std::function<void()>& restoreFocus);
    void openModelPicker(ComposerSessionController* composer,
                         const std::function<void()>& focusComposer);
    // Local model track (Phase 2): a two-step download flow - pick a repo from discovery, then a
    // quant file from that repo (recommended row starred), kicking ModelDownload. `onChange` is
    // invoked after a download starts so the caller can refresh the page.
    void openModelDownload(models::IModelCatalog* catalog, const std::function<void()>& onChange);
    void openCommandPalette(CommandRegistry* commands, const CommandCallbacks& callbacks);
    [[nodiscard]] class QuitDialog* quitDialog() const { return m_quitDialog; }

private:
    Tui::ZWidget* m_parent = nullptr;
    class QuitDialog* m_quitDialog = nullptr;
    PaletteDialog* m_modelPicker = nullptr;
    PaletteDialog* m_modelDiscover = nullptr;
    PaletteDialog* m_quantPicker = nullptr;
    PaletteDialog* m_commandPalette = nullptr;
};
