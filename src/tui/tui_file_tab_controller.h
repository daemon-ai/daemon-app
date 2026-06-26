#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <Tui/ZLabel.h>

namespace editor {
class CodeEditorController;
}
namespace fs {
class IFsService;
}

class CodeEditorView;
class TabModel;

class TuiFileTabController : public QObject {
    Q_OBJECT

public:
    TuiFileTabController(fs::IFsService* fs, TabModel* tabs, QObject* parent = nullptr);

    void setStatusLabel(Tui::ZLabel* label);

    editor::CodeEditorController* ensureFileSession(int tabId);
    bool destroySession(int tabId, CodeEditorView* editorView);
    void saveActive(CodeEditorView* editorView);
    void setDarkTheme(bool dark);

private:
    [[nodiscard]] QString keyFor(const QString& rootId, const QString& path) const;

    fs::IFsService* m_fs = nullptr;
    TabModel* m_tabs = nullptr;
    Tui::ZLabel* m_status = nullptr;
    QHash<int, editor::CodeEditorController*> m_fileSessions;
    QHash<QString, editor::CodeEditorController*> m_fileByKey;
    QString m_activeRoot;
    QString m_activePath;
};
