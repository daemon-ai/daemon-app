// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "tui_overlay_host.h"

#include "command_registry.h"
#include "composer_session_controller.h"
#include "models/imodel_catalog.h"
#include "tui_dialogs.h"
#include "uimodels/variant_list_model.h"

#include <QCoreApplication>
#include <QVector>
#include <Tui/ZDialog.h>
#include <Tui/ZWidget.h>

TuiOverlayHost::TuiOverlayHost(Tui::ZWidget* parent) : QObject(parent), m_parent(parent) {}

void TuiOverlayHost::promptQuit(const std::function<void()>& restoreFocus) {
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
                                     const std::function<void()>& focusComposer) {
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

void TuiOverlayHost::openModelDownload(models::IModelCatalog* catalog,
                                       const std::function<void()>& onChange) {
    if (catalog == nullptr) {
        return;
    }
    if (m_modelDiscover == nullptr) {
        m_modelDiscover = new PaletteDialog(tr("Download model \u2014 pick a repo"), m_parent);
    }
    if (m_quantPicker == nullptr) {
        m_quantPicker = new PaletteDialog(tr("Choose a quantization"), m_parent);
    }
    // Rewire each open to capture the current catalog/callbacks (cheap, avoids stale lambdas).
    disconnect(m_modelDiscover, &PaletteDialog::activated, this, nullptr);
    connect(
        m_modelDiscover, &PaletteDialog::activated, this,
        [this, catalog, onChange](const QString& repo) {
            // Step 2: request the repo's files; populate + open the quant palette when they
            // arrive (single-shot, so a later repo pick doesn't double-fire).
            catalog->repoFiles(repo);
            disconnect(m_quantPicker, &PaletteDialog::activated, this, nullptr);
            connect(m_quantPicker, &PaletteDialog::activated, this,
                    [catalog, repo, onChange](const QString& file) {
                        catalog->downloadFile(repo, file);
                        if (onChange) {
                            onChange();
                        }
                    });
            auto* conn = new QMetaObject::Connection;
            *conn = connect(
                catalog, &models::IModelCatalog::filesChanged, this,
                [this, catalog, repo, conn](const QString& loadedRepo) {
                    if (loadedRepo != repo) {
                        return;
                    }
                    QObject::disconnect(*conn);
                    delete conn;
                    QVector<PaletteDialog::Item> items;
                    auto* files = qobject_cast<uimodels::VariantListModel*>(catalog->files());
                    const QVariantMap rec = catalog->recommendation();
                    const QString recFile = rec.value(QStringLiteral("file")).toString();
                    if (files != nullptr) {
                        for (const QVariantMap& f : files->rows()) {
                            const QString path = f.value(QStringLiteral("path")).toString();
                            const bool recommended = (path == recFile);
                            items.push_back(
                                {path,
                                 (recommended ? QStringLiteral("\u2605 ") : QStringLiteral("  ")) +
                                     f.value(QStringLiteral("quant")).toString() +
                                     QStringLiteral("  \u00b7  ") +
                                     f.value(QStringLiteral("sizeLabel")).toString(),
                                 path});
                        }
                    }
                    m_quantPicker->setItems(items);
                    m_quantPicker->openCentered();
                });
        });

    // Seed the repo palette from the current discovery results; kick a default search if empty so
    // a first open is not blank (the user can type to filter, then reopen for the fresh set).
    auto* disc = qobject_cast<uimodels::VariantListModel*>(catalog->discover());
    if (disc == nullptr || disc->rows().isEmpty()) {
        catalog->search(QString());
    }
    QVector<PaletteDialog::Item> items;
    if (disc != nullptr) {
        for (const QVariantMap& r : disc->rows()) {
            items.push_back({r.value(QStringLiteral("repo")).toString(),
                             r.value(QStringLiteral("name")).toString(),
                             r.value(QStringLiteral("params")).toString()});
        }
    }
    m_modelDiscover->setItems(items);
    m_modelDiscover->openCentered();
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
