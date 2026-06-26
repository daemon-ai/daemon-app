#include "core/command_stack.h"

namespace be {

bool EditorCommand::mergeWith(const EditorCommand&) {
    return false;
}

ReplaceBlockCommand::ReplaceBlockCommand(BlockId id, QString before, QString after)
    : m_id(id), m_before(std::move(before)), m_after(std::move(after)) {}

void ReplaceBlockCommand::redo(DocumentStore& store) {
    store.replaceBlockMarkdown(m_id, m_after);
}

void ReplaceBlockCommand::undo(DocumentStore& store) {
    store.replaceBlockMarkdown(m_id, m_before);
}

bool ReplaceBlockCommand::mergeWith(const EditorCommand& next) {
    const auto* replace = dynamic_cast<const ReplaceBlockCommand*>(&next);
    if (!replace || replace->m_id != m_id) {
        return false;
    }
    m_after = replace->m_after;
    return true;
}

DeleteBlocksCommand::DeleteBlocksCommand(qsizetype row, QVector<BlockRecord> removed)
    : m_row(row), m_removed(std::move(removed)) {}

void DeleteBlocksCommand::redo(DocumentStore& store) {
    store.deleteBlocks(m_row, m_removed.size());
}

void DeleteBlocksCommand::undo(DocumentStore& store) {
    BlockId anchor = 0;
    if (m_row > 0) {
        if (const BlockRecord* previous = store.blockAt(m_row - 1)) {
            anchor = previous->id;
        }
    }

    for (const BlockRecord& block : m_removed) {
        if (anchor == 0 && store.blockCount() > 0) {
            store.insertBlockAfter(store.blockAt(0)->id, block.markdown());
        } else if (anchor != 0) {
            store.insertBlockAfter(anchor, block.markdown());
        } else {
            store.loadMarkdown(block.markdown());
        }
    }
}

MoveBlocksCommand::MoveBlocksCommand(qsizetype firstRow, qsizetype count, qsizetype destinationRow)
    : m_firstRow(firstRow), m_count(count), m_destinationRow(destinationRow) {}

void MoveBlocksCommand::redo(DocumentStore& store) {
    store.moveBlocks(m_firstRow, m_count, m_destinationRow);
}

void MoveBlocksCommand::undo(DocumentStore& store) {
    const qsizetype movedStart =
        m_destinationRow > m_firstRow ? m_destinationRow - m_count : m_destinationRow;
    store.moveBlocks(movedStart, m_count, m_firstRow);
}

StructuralCommand::StructuralCommand(QVector<BlockRecord> before, QVector<BlockRecord> after)
    : m_before(std::move(before)), m_after(std::move(after)) {}

void StructuralCommand::redo(DocumentStore& store) {
    store.restore(m_after);
}

void StructuralCommand::undo(DocumentStore& store) {
    store.restore(m_before);
}

StreamCommand::StreamCommand(qsizetype firstRow, QVector<BlockRecord> before,
                             QVector<BlockRecord> after)
    : m_firstRow(firstRow), m_before(std::move(before)), m_after(std::move(after)) {}

void StreamCommand::redo(DocumentStore& store) {
    store.spliceBlocks(m_firstRow, m_before.size(), m_after);
}

void StreamCommand::undo(DocumentStore& store) {
    store.spliceBlocks(m_firstRow, m_after.size(), m_before);
}

void CommandStack::push(std::unique_ptr<EditorCommand> command, DocumentStore& store) {
    if (!command) {
        return;
    }

    if (!m_undo.empty() && m_undo.back()->mergeWith(*command)) {
        m_undo.back()->redo(store);
    } else {
        command->redo(store);
        m_undo.push_back(std::move(command));
    }
    m_redo.clear();
}

void CommandStack::pushApplied(std::unique_ptr<EditorCommand> command) {
    if (!command) {
        return;
    }
    m_undo.push_back(std::move(command));
    m_redo.clear();
}

bool CommandStack::canUndo() const {
    return !m_undo.empty();
}

bool CommandStack::canRedo() const {
    return !m_redo.empty();
}

void CommandStack::undo(DocumentStore& store) {
    if (m_undo.empty()) {
        return;
    }
    auto command = std::move(m_undo.back());
    m_undo.pop_back();
    command->undo(store);
    m_redo.push_back(std::move(command));
}

void CommandStack::redo(DocumentStore& store) {
    if (m_redo.empty()) {
        return;
    }
    auto command = std::move(m_redo.back());
    m_redo.pop_back();
    command->redo(store);
    m_undo.push_back(std::move(command));
}

void CommandStack::clear() {
    m_undo.clear();
    m_redo.clear();
}

} // namespace be
