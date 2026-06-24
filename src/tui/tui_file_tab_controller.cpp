#include "tui_file_tab_controller.h"

#include "code_editor_view.h"
#include "tab_model.h"
#include "tui_palette.h"

#include "app/code_editor_controller.h"
#include "fs/ifs_service.h"

#include <Tui/ZLabel.h>

#include <utility>

TuiFileTabController::TuiFileTabController(fs::IFsService* fs, TabModel* tabs, QObject* parent)
    : QObject(parent)
    , m_fs(fs)
    , m_tabs(tabs)
{
    if (m_fs == nullptr) {
        return;
    }
    connect(m_fs, &fs::IFsService::fileRead, this,
            [this](const QString& rootId, const QString& path, const QByteArray& bytes,
                   const QString& revision, bool binary, bool /*truncated*/) {
                editor::CodeEditorController* c = m_fileByKey.value(keyFor(rootId, path), nullptr);
                if (c != nullptr && !binary) {
                    c->loadBytes(bytes, path, revision);
                    if (m_status != nullptr)
                        m_status->setText(QString());
                } else if (c != nullptr && m_status != nullptr) {
                    m_status->setText(QStringLiteral("Binary file - not editable"));
                }
            });
    connect(m_fs, &fs::IFsService::writeResult, this,
            [this](const QString& rootId, const QString& path, bool ok, const QString& revision,
                   const QString& error) {
                editor::CodeEditorController* c = m_fileByKey.value(keyFor(rootId, path), nullptr);
                if (c != nullptr && ok) {
                    c->markSaved(revision);
                    if (m_status != nullptr)
                        m_status->setText(QStringLiteral("Saved"));
                } else if (c != nullptr && m_status != nullptr) {
                    m_status->setText(error.isEmpty() ? QStringLiteral("Save failed") : error);
                }
            });
}

void TuiFileTabController::setStatusLabel(Tui::ZLabel* label)
{
    m_status = label;
}

QString TuiFileTabController::keyFor(const QString& rootId, const QString& path) const
{
    return rootId + QChar(0x1f) + path;
}

editor::CodeEditorController* TuiFileTabController::ensureFileSession(int tabId)
{
    if (m_fileSessions.contains(tabId))
        return m_fileSessions.value(tabId);
    if (m_tabs == nullptr)
        return nullptr;
    const int row = m_tabs->indexOfTabId(tabId);
    if (row < 0)
        return nullptr;
    const QString rootId = m_tabs->fileRootAt(row);
    const QString path = m_tabs->filePathAt(row);
    m_activeRoot = rootId;
    m_activePath = path;

    auto* c = new editor::CodeEditorController(this);
    c->setDarkTheme(tpal::activeTheme() != theme::ThemeName::Light
                    && tpal::activeTheme() != theme::ThemeName::Sepia);
    m_fileSessions.insert(tabId, c);
    m_fileByKey.insert(keyFor(rootId, path), c);
    connect(c, &editor::CodeEditorController::modifiedChanged, this,
            [this, tabId, c] {
                if (m_tabs != nullptr)
                    m_tabs->setDirtyById(tabId, c->modified());
            });
    if (m_fs != nullptr)
        m_fs->read(rootId, path);
    return c;
}

bool TuiFileTabController::destroySession(int tabId, CodeEditorView* editorView)
{
    auto fit = m_fileSessions.find(tabId);
    if (fit == m_fileSessions.end()) {
        return false;
    }
    editor::CodeEditorController* c = fit.value();
    if (editorView != nullptr && editorView->controller() == c)
        editorView->setController(nullptr);
    for (auto kit = m_fileByKey.begin(); kit != m_fileByKey.end();) {
        if (kit.value() == c)
            kit = m_fileByKey.erase(kit);
        else
            ++kit;
    }
    m_fileSessions.erase(fit);
    delete c;
    return true;
}

void TuiFileTabController::saveActive(CodeEditorView* editorView)
{
    if (m_fs != nullptr && editorView != nullptr && editorView->controller() != nullptr
        && !m_activePath.isEmpty()) {
        m_fs->write(m_activeRoot, m_activePath, editorView->controller()->textBytes(),
                    editorView->controller()->revision(), false);
    }
}

void TuiFileTabController::setDarkTheme(bool dark)
{
    for (editor::CodeEditorController* c : std::as_const(m_fileSessions)) {
        c->setDarkTheme(dark);
    }
}
