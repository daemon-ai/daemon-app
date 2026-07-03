// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "palette_dialog.h"

#include <functional>
#include <QObject>
#include <Tui/ZWidget.h>

class CommandRegistry;
class ComposerSessionController;
class FileFinderDialog;
namespace files {
class FileFinderModel;
}
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
    // Ctrl+G "go to file": fuzzy finder over the shared FileFinderModel. Enter
    // reports pinned=false (preview tab), Shift+Enter pinned=true.
    void openFileFinder(files::FileFinderModel* model,
                        const std::function<void(const QString& rootId, const QString& path,
                                                 bool pinned)>& onChosen);
    // Ctrl+O composer attach: the same finder styled as a workspace file picker
    // (the TUI analog of the GUI's FilePicker). `restoreFocus` runs after a pick
    // AND after a cancel, so the composer regains focus either way.
    void openAttachPicker(
        files::FileFinderModel* model,
        const std::function<void(const QString& rootId, const QString& path)>& onPicked,
        const std::function<void()>& restoreFocus);
    [[nodiscard]] class QuitDialog* quitDialog() const { return m_quitDialog; }

private:
    Tui::ZWidget* m_parent = nullptr;
    class QuitDialog* m_quitDialog = nullptr;
    PaletteDialog* m_modelPicker = nullptr;
    PaletteDialog* m_modelDiscover = nullptr;
    PaletteDialog* m_quantPicker = nullptr;
    PaletteDialog* m_commandPalette = nullptr;
    FileFinderDialog* m_fileFinder = nullptr;
    FileFinderDialog* m_attachPicker = nullptr;
};
