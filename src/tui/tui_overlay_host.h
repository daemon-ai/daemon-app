#pragma once

#include "palette_dialog.h"

#include <Tui/ZWidget.h>

#include <QObject>

#include <functional>

class CommandRegistry;
class ComposerSessionController;

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

    void promptQuit(std::function<void()> restoreFocus);
    void openModelPicker(ComposerSessionController* composer, std::function<void()> focusComposer);
    void openCommandPalette(CommandRegistry* commands, const CommandCallbacks& callbacks);
    [[nodiscard]] class QuitDialog* quitDialog() const { return m_quitDialog; }

private:
    Tui::ZWidget* m_parent = nullptr;
    class QuitDialog* m_quitDialog = nullptr;
    PaletteDialog* m_modelPicker = nullptr;
    PaletteDialog* m_commandPalette = nullptr;
};
