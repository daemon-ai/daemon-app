#include "composer_session_controller.h"

#include <QRegularExpression>

ComposerSessionController::ComposerSessionController(QObject* parent)
    : QObject(parent)
    , m_queue(new ComposerQueueModel(this))
    , m_attachments(new ComposerAttachmentModel(this))
    , m_completion(new CompletionModel(this))
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
    closeTrigger(); // a completion popover never crosses a conversation boundary

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

void ComposerSessionController::applyDraftWithCursor(const QString& text, int cursor)
{
    m_draft = text;
    emit draftReset(text); // view replaces its text (caret-to-end)...
    emit cursorRequested(cursor); // ...then we override the caret to `cursor`
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
    closeTrigger();
}

void ComposerSessionController::refreshTrigger(const QString& text, int cursorPos)
{
    const int caret = qBound(0, cursorPos, text.length());
    const QString before = text.left(caret);

    // Match a trigger token at the caret: start-of-line or whitespace, then a
    // single / or @, then the (whitespace-free) query. Mirrors the QML regex.
    static const QRegularExpression re(QStringLiteral("(^|\\s)([/@])(\\S*)$"));
    const QRegularExpressionMatch m = re.match(before);
    if (!m.hasMatch()) {
        closeTrigger();
        return;
    }

    const QString kind =
        m.captured(2) == QStringLiteral("@") ? QStringLiteral("mention") : QStringLiteral("slash");
    const QString query = m.captured(3);
    const QList<CompletionModel::Item> items = CompletionModel::search(kind, query);
    if (items.isEmpty()) {
        closeTrigger();
        return;
    }

    m_triggerStart = caret - (query.length() + 1); // include the trigger char
    m_triggerEnd = caret;
    m_completion->setItems(items);
    setActiveIndex(0);
    if (m_completionKind != kind) {
        m_completionKind = kind;
        emit completionKindChanged();
    }
    if (!m_completionActive) {
        m_completionActive = true;
        emit completionActiveChanged();
    }
}

void ComposerSessionController::closeTrigger()
{
    m_triggerStart = -1;
    m_triggerEnd = -1;
    if (m_completionActive) {
        m_completionActive = false;
        emit completionActiveChanged();
    }
}

void ComposerSessionController::moveActive(int delta)
{
    const int n = m_completion->count();
    if (!m_completionActive || n == 0) {
        return;
    }
    setActiveIndex(((m_completionActiveIndex + delta) % n + n) % n);
}

void ComposerSessionController::setActiveIndex(int index)
{
    if (m_completionActiveIndex == index) {
        return;
    }
    m_completionActiveIndex = index;
    emit completionActiveIndexChanged();
}

void ComposerSessionController::acceptActive()
{
    accept(m_completionActiveIndex);
}

void ComposerSessionController::accept(int index)
{
    const int n = m_completion->count();
    if (index < 0 || index >= n) {
        closeTrigger();
        return;
    }
    const CompletionModel::Item item = m_completion->at(index);

    const int start = m_triggerStart >= 0 ? m_triggerStart : m_draft.length();
    const int end = m_triggerEnd >= 0 ? m_triggerEnd : start;
    const int s = qBound(0, start, m_draft.length());
    const int e = qBound(s, end, m_draft.length());

    if (item.action != QStringLiteral("insert")) {
        // Strip the typed trigger token, then run the command.
        const QString next = m_draft.left(s) + m_draft.mid(e);
        closeTrigger();
        applyDraftWithCursor(next, s);
        if (item.action == QStringLiteral("clear")) {
            clear();
        } else {
            invokeCommand(item.action);
        }
        return;
    }

    // Insert: replace the trigger token with the item value.
    const QString next = m_draft.left(s) + item.value + m_draft.mid(e);
    const int caret = s + item.value.length();
    closeTrigger();
    applyDraftWithCursor(next, caret);
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
