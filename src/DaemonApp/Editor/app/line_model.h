#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace editor {

class TextDocument;
class CodeHighlighter;

// The view-facing line model: one row per VISIBLE document line (folded ranges
// are hidden). It serves raw text, structured syntax style runs, and fold flags.
// Owned by CodeEditorController; not creatable from QML directly.
class LineModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Obtained from CodeEditorController.lines")

public:
    enum Role {
        TextRole = Qt::UserRole + 1, // raw line text
        StyleRunsRole,               // list of {start,length,color,bold,italic}
        FoldStartRole,               // a fold region begins here
        FoldedRole,                  // this fold start is collapsed
        RealLineRole,                // document line index
    };
    Q_ENUM(Role)

    LineModel(TextDocument* doc, CodeHighlighter* hl, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] Q_INVOKABLE int realLine(int row) const;
    [[nodiscard]] Q_INVOKABLE int rowForLine(int line) const;

    // Toggle the fold whose region starts at document line `realLine`.
    void toggleFold(int realLine);
    void resetFolds();

private slots:
    void onLineChanged(int line);
    void onLinesReset();
    void onHighlightingChanged(int firstLine, int lastLine);

private:
    void rebuildVisible();
    [[nodiscard]] int foldRegionEnd(int startLine) const; // indentation heuristic
    [[nodiscard]] QVariantList styleRunsForLine(int line) const;

    TextDocument* m_doc = nullptr;
    CodeHighlighter* m_hl = nullptr;
    QList<int> m_visible; // visible row -> document line
    QSet<int> m_folded;   // collapsed fold-start lines
};

} // namespace editor
