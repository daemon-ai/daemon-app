#pragma once

#include "core/document_store.h"

#include <memory>
#include <vector>

namespace be {

class EditorCommand {
public:
    virtual ~EditorCommand() = default;
    virtual void redo(DocumentStore& store) = 0;
    virtual void undo(DocumentStore& store) = 0;
    virtual bool mergeWith(const EditorCommand& next);
};

class ReplaceBlockCommand final : public EditorCommand {
public:
    ReplaceBlockCommand(BlockId id, QString before, QString after);

    void redo(DocumentStore& store) override;
    void undo(DocumentStore& store) override;
    bool mergeWith(const EditorCommand& next) override;

private:
    BlockId m_id = 0;
    QString m_before;
    QString m_after;
};

class DeleteBlocksCommand final : public EditorCommand {
public:
    DeleteBlocksCommand(qsizetype row, QVector<BlockRecord> removed);

    void redo(DocumentStore& store) override;
    void undo(DocumentStore& store) override;

private:
    qsizetype m_row = 0;
    QVector<BlockRecord> m_removed;
};

class MoveBlocksCommand final : public EditorCommand {
public:
    MoveBlocksCommand(qsizetype firstRow, qsizetype count, qsizetype destinationRow);

    void redo(DocumentStore& store) override;
    void undo(DocumentStore& store) override;

private:
    qsizetype m_firstRow = 0;
    qsizetype m_count = 0;
    qsizetype m_destinationRow = 0;
};

// Snapshot-based undo for structural edits (split/merge/insert/delete/move).
// Captures the whole block list before and after the edit so identity, ordering,
// and canonical serialization are restored exactly.
class StructuralCommand final : public EditorCommand {
public:
    StructuralCommand(QVector<BlockRecord> before, QVector<BlockRecord> after);

    void redo(DocumentStore& store) override;
    void undo(DocumentStore& store) override;

private:
    QVector<BlockRecord> m_before;
    QVector<BlockRecord> m_after;
};

// Scoped undo for an append-at-end streaming session. Captures only the affected
// tail range (rows replaced/inserted by the stream) rather than a whole-document
// snapshot, so memory does not grow with stream length.
class StreamCommand final : public EditorCommand {
public:
    StreamCommand(qsizetype firstRow, QVector<BlockRecord> before, QVector<BlockRecord> after);

    void redo(DocumentStore& store) override;
    void undo(DocumentStore& store) override;

private:
    qsizetype m_firstRow = 0;
    QVector<BlockRecord> m_before;
    QVector<BlockRecord> m_after;
};

class CommandStack {
public:
    void push(std::unique_ptr<EditorCommand> command, DocumentStore& store);
    // Record an already-applied command for undo/redo without re-running redo()
    // (used by streaming, whose effect is applied incrementally as tokens arrive).
    void pushApplied(std::unique_ptr<EditorCommand> command);
    bool canUndo() const;
    bool canRedo() const;
    void undo(DocumentStore& store);
    void redo(DocumentStore& store);
    void clear();

private:
    std::vector<std::unique_ptr<EditorCommand>> m_undo;
    std::vector<std::unique_ptr<EditorCommand>> m_redo;
};

} // namespace be
