#include "tui_overlay_host.h"

#include "command_registry.h"
#include "composer_session_controller.h"
#include "tui_dialogs.h"

#include <QCoreApplication>
#include <Tui/ZDialog.h>
#include <Tui/ZWidget.h>

TuiOverlayHost::TuiOverlayHost(Tui::ZWidget* parent) : QObject(parent), m_parent(parent) {}

void TuiOverlayHost::promptQuit(std::function<void()> restoreFocus) {
    if (m_quitDialog != nullptr) {
        return;
    }
    m_quitDialog = new QuitDialog(m_parent);

    connect(m_quitDialog, &QuitDialog::quitConfirmed, this, [] { QCoreApplication::quit(); });
    connect(m_quitDialog, &Tui::ZDialog::rejected, this, [this, restoreFocus] {
        m_quitDialog = nullptr; // DeleteOnClose frees it
        if (restoreFocus) {
            restoreFocus();
        }
    });
}

void TuiOverlayHost::openModelPicker(ComposerSessionController* composer,
                                     std::function<void()> focusComposer) {
    if (composer == nullptr) {
        return;
    }
    if (m_modelPicker == nullptr) {
        m_modelPicker = new PaletteDialog(tr("Model"), m_parent);
        connect(m_modelPicker, &PaletteDialog::activated, this,
                [composer, focusComposer](const QString& id) {
                    bool ok = false;
                    const int index = id.toInt(&ok);
                    if (ok) {
                        composer->selectModel(index);
                    }
                    if (focusComposer) {
                        focusComposer();
                    }
                });
    }
    const QVariantList catalog = composer->modelCatalog();
    QVector<PaletteDialog::Item> items;
    items.reserve(catalog.size());
    for (int i = 0; i < catalog.size(); ++i) {
        const QVariantMap m = catalog.at(i).toMap();
        const bool current = i == composer->currentModelIndex();
        items.push_back({QString::number(i),
                         (current ? QStringLiteral("✓ ") : QStringLiteral("  ")) +
                             m.value(QStringLiteral("label")).toString(),
                         m.value(QStringLiteral("provider")).toString()});
    }
    m_modelPicker->setItems(items);
    m_modelPicker->openCentered();
}

void TuiOverlayHost::openCommandPalette(CommandRegistry* commands,
                                        const CommandCallbacks& callbacks) {
    if (commands == nullptr) {
        return;
    }
    if (m_commandPalette == nullptr) {
        m_commandPalette = new PaletteDialog(tr("Commands"), m_parent);
        connect(m_commandPalette, &PaletteDialog::activated, this, [callbacks](const QString& id) {
            if (callbacks.openManagerPage && callbacks.openManagerPage(id)) {
                // routed to a page tab
            } else if (id == QStringLiteral("new")) {
                if (callbacks.newTranscriptTab)
                    callbacks.newTranscriptTab();
            } else if (id == QStringLiteral("theme")) {
                if (callbacks.cycleTheme)
                    callbacks.cycleTheme();
            } else if (id == QStringLiteral("model")) {
                if (callbacks.openModelPicker)
                    callbacks.openModelPicker();
            } else if (id == QStringLiteral("search")) {
                if (callbacks.focusSearch)
                    callbacks.focusSearch();
            } else if (id == QStringLiteral("files")) {
                if (callbacks.toggleExplorer)
                    callbacks.toggleExplorer();
            } else if (id == QStringLiteral("find")) {
                if (callbacks.openTranscriptSearch)
                    callbacks.openTranscriptSearch();
            } else if (callbacks.invokeActiveCommand) {
                callbacks.invokeActiveCommand(id);
            }
            if (callbacks.focusComposer) {
                callbacks.focusComposer();
            }
        });
    }
    commands->search(QString());
    const QVector<CommandRegistry::Command> cmds = commands->visibleCommands();
    QVector<PaletteDialog::Item> items;
    items.reserve(cmds.size());
    for (const CommandRegistry::Command& c : cmds) {
        QString hint = c.group;
        if (!c.shortcut.isEmpty()) {
            hint += QStringLiteral("  ") + c.shortcut;
        }
        items.push_back({c.id, c.title, hint});
    }
    m_commandPalette->setItems(items);
    m_commandPalette->openCentered();
}
