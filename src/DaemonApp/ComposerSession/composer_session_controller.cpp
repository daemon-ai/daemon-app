#include "composer_session_controller.h"

ComposerSessionController::ComposerSessionController(QObject* parent)
    : QObject(parent)
    , m_queue(new ComposerQueueModel(this))
    , m_attachments(new ComposerAttachmentModel(this))
{
    // The queue count feeds the queueCount property; attachments feed the derived
    // hasPayload/canSteer flags.
    connect(m_queue, &ComposerQueueModel::countChanged, this,
            &ComposerSessionController::queueCountChanged);
    connect(m_attachments, &ComposerAttachmentModel::countChanged, this,
            &ComposerSessionController::derivedChanged);
}

void ComposerSessionController::setConversationId(int id)
{
    if (m_conversationId == id) {
        return;
    }
    // Editing/browsing state never crosses a conversation boundary.
    if (m_editingIndex != -1) {
        m_editingIndex = -1;
        emit editingIndexChanged();
    }
    m_preEditDraft.clear();
    resetBrowse();

    stash(m_conversationId);
    m_conversationId = id;
    restore(id);
    emit conversationIdChanged();
}

void ComposerSessionController::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
    emit derivedChanged(); // canSteer depends on busy
    // Flow queued turns whenever the session goes idle.
    if (!m_busy && m_queue->count() > 0) {
        drainNext();
    }
}

void ComposerSessionController::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    emit enabledChanged();
}

void ComposerSessionController::setDraft(const QString& draft)
{
    if (m_draft == draft) {
        return;
    }
    m_draft = draft;
    emit draftChanged();
    emit derivedChanged();
}

void ComposerSessionController::applyDraft(const QString& text)
{
    m_draft = text;
    emit draftReset(text); // view replaces its text + moves caret to end
    emit draftChanged();
    emit derivedChanged();
}

bool ComposerSessionController::hasPayload() const
{
    return m_draft.trimmed().length() > 0 || m_attachments->count() > 0;
}

bool ComposerSessionController::canSteer() const
{
    return m_busy && m_draft.trimmed().length() > 0 && m_attachments->count() == 0;
}

void ComposerSessionController::resetBrowse()
{
    m_browseIndex = -1;
    m_historyDraft.clear();
}

void ComposerSessionController::pushHistory(const QString& text)
{
    QStringList arr = m_histories.value(m_conversationId);
    if (arr.isEmpty() || arr.last() != text) {
        arr.push_back(text);
    }
    if (arr.size() > 100) {
        arr = arr.mid(arr.size() - 100);
    }
    m_histories[m_conversationId] = arr;
}

void ComposerSessionController::enqueue(const QString& text, const QString& refs)
{
    m_queue->append(text, refs);
}

void ComposerSessionController::drainNext()
{
    if (m_draining || m_busy || m_queue->count() == 0) {
        return;
    }
    m_draining = true;
    const QString text = m_queue->textAt(0);
    const QString refs = m_queue->refsAt(0);
    m_queue->removeAt(0);
    if (m_editingIndex >= 0) {
        setEditingIndex(m_editingIndex - 1);
    }
    pushHistory(text);
    emit submitted(text, refs);
    m_draining = false;
}

void ComposerSessionController::submit()
{
    if (!m_enabled) {
        return;
    }
    const QString text = m_draft.trimmed();

    // Saving an in-place queue edit (not a send).
    if (m_editingIndex >= 0) {
        if (!text.isEmpty()) {
            m_queue->setEntry(m_editingIndex, text, m_attachments->buildRefs());
        }
        exitEdit(false);
        return;
    }

    if (!hasPayload()) {
        // Empty Enter with a non-empty queue drains the next entry.
        if (!m_busy && m_queue->count() > 0) {
            drainNext();
        }
        return;
    }

    const QString refs = m_attachments->buildRefs();

    if (m_busy) {
        // Mid-turn: queue the draft instead of sending.
        enqueue(text, refs);
        clear();
        resetBrowse();
        return;
    }

    pushHistory(text);
    clear();
    resetBrowse();
    emit submitted(text, refs);
}

void ComposerSessionController::enqueueDraft()
{
    if (!hasPayload()) {
        return;
    }
    enqueue(m_draft.trimmed(), m_attachments->buildRefs());
    clear();
    resetBrowse();
}

void ComposerSessionController::steerDraft()
{
    if (!canSteer()) {
        return;
    }
    const QString text = m_draft.trimmed();
    clear();
    resetBrowse();
    emit steer(text);
}

void ComposerSessionController::cancel()
{
    emit cancelRequested();
}

bool ComposerSessionController::browseUp()
{
    const QStringList arr = m_histories.value(m_conversationId);
    if (arr.isEmpty()) {
        return false;
    }
    if (m_browseIndex == -1) {
        m_historyDraft = m_draft;
        m_browseIndex = arr.size() - 1;
    } else if (m_browseIndex > 0) {
        m_browseIndex -= 1;
    } else {
        return true; // at the oldest entry; swallow so the caret doesn't move
    }
    applyDraft(arr.at(m_browseIndex));
    return true;
}

bool ComposerSessionController::browseDown()
{
    if (m_browseIndex == -1) {
        return false;
    }
    const QStringList arr = m_histories.value(m_conversationId);
    if (m_browseIndex < arr.size() - 1) {
        m_browseIndex += 1;
        applyDraft(arr.at(m_browseIndex));
    } else {
        m_browseIndex = -1;
        applyDraft(m_historyDraft);
    }
    return true;
}

void ComposerSessionController::sendNowEntry(int index)
{
    if (index < 0 || index >= m_queue->count()) {
        return;
    }
    if (m_busy) {
        // Promote to the head and interrupt; the busy->idle drain sends it.
        if (index != 0) {
            const QString text = m_queue->textAt(index);
            const QString refs = m_queue->refsAt(index);
            m_queue->removeAt(index);
            m_queue->insertAt(0, text, refs);
        }
        emit cancelRequested();
        return;
    }
    const QString text = m_queue->textAt(index);
    const QString refs = m_queue->refsAt(index);
    m_queue->removeAt(index);
    pushHistory(text);
    emit submitted(text, refs);
}

void ComposerSessionController::deleteEntry(int index)
{
    if (index < 0 || index >= m_queue->count()) {
        return;
    }
    if (index == m_editingIndex) {
        exitEdit(true);
    } else if (index < m_editingIndex) {
        setEditingIndex(m_editingIndex - 1);
    }
    m_queue->removeAt(index);
}

void ComposerSessionController::beginEdit(int index)
{
    if (index < 0 || index >= m_queue->count() || m_editingIndex >= 0) {
        return;
    }
    m_preEditDraft = m_draft;
    setEditingIndex(index);
    applyDraft(m_queue->textAt(index));
}

void ComposerSessionController::exitEdit(bool restore)
{
    if (m_editingIndex < 0) {
        return;
    }
    setEditingIndex(-1);
    applyDraft(restore ? m_preEditDraft : QString());
    m_preEditDraft.clear();
}

void ComposerSessionController::addAttachment(const QString& name, const QString& kind)
{
    m_attachments->append(name, kind);
}

void ComposerSessionController::invokeCommand(const QString& command)
{
    emit commandInvoked(command);
}

void ComposerSessionController::clear()
{
    applyDraft(QString());
    m_attachments->clear();
}

void ComposerSessionController::stash(int key)
{
    m_drafts[key] = m_draft;
    m_queues[key] = m_queue->entries();
}

void ComposerSessionController::restore(int key)
{
    applyDraft(m_drafts.value(key));
    m_queue->setEntries(m_queues.value(key));
}

void ComposerSessionController::setEditingIndex(int index)
{
    if (m_editingIndex == index) {
        return;
    }
    m_editingIndex = index;
    emit editingIndexChanged();
}
