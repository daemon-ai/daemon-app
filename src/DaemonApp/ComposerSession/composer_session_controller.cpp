#include "composer_session_controller.h"

#include "models/imodel_catalog.h"
#include "uimodels/variant_list_model.h"

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

    // Flat label list, in catalog order, so `currentModelIndex` addresses the same
    // model whether a view reads the flat list or the structured catalog.
    m_models.reserve(m_catalog.size());
    for (const ModelEntry& e : m_catalog) {
        m_models << e.label;
    }
}

void ComposerSessionController::setSessionId(const QString& id)
{
    if (m_sessionId == id) {
        return;
    }
    // Editing/browsing state never crosses a session boundary.
    if (m_editingIndex != -1) {
        m_editingIndex = -1;
        emit editingIndexChanged();
    }
    m_preEditDraft.clear();
    resetBrowse();
    resetReverseSearch(); // an in-flight Ctrl+R never crosses a session boundary
    closeTrigger(); // a completion popover never crosses a session boundary

    stash(m_sessionId);
    m_sessionId = id;
    restore(id);
    emit sessionIdChanged();
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
    emit derivedChanged(); // primaryActionEnabled folds in `enabled`
}

void ComposerSessionController::setDraft(const QString& draft)
{
    if (m_draft == draft) {
        return;
    }
    // A live edit that isn't the reverse-search preview echo (the preview sets
    // m_draft before emitting draftReset, so its echo hits the early-return above)
    // means the user is editing the field directly; drop out of reverse search.
    resetReverseSearch();
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

QString ComposerSessionController::primaryAction() const
{
    if (m_busy) {
        return hasPayload() ? QStringLiteral("queue") : QStringLiteral("stop");
    }
    return QStringLiteral("send");
}

bool ComposerSessionController::primaryActionEnabled() const
{
    // Stop/queue are always actionable while enabled; send needs a payload.
    return m_enabled && (primaryAction() != QStringLiteral("send") || hasPayload());
}

QVariantList ComposerSessionController::catalogEntries() const
{
    // Injected source: project the installed models into the {provider,id,label}
    // shape the GUI picker + TUI expect (label = the model's display name).
    if (m_modelSource != nullptr) {
        auto* installed = qobject_cast<uimodels::VariantListModel*>(m_modelSource->installed());
        QVariantList out;
        if (installed != nullptr) {
            const QList<QVariantMap>& rows = installed->rows();
            out.reserve(rows.size());
            for (const QVariantMap& r : rows) {
                const QString name = r.value(QStringLiteral("name")).toString();
                const QString id = r.value(QStringLiteral("id")).toString();
                out.append(QVariantMap {
                    { QStringLiteral("provider"), r.value(QStringLiteral("provider")) },
                    { QStringLiteral("id"), id },
                    { QStringLiteral("label"), name.isEmpty() ? id : name },
                });
            }
        }
        return out;
    }

    // Fallback (no daemon/catalog wired): the built-in canned cloud catalog.
    QVariantList out;
    out.reserve(m_catalog.size());
    for (const ModelEntry& e : m_catalog) {
        out.append(QVariantMap {
            { QStringLiteral("provider"), e.provider },
            { QStringLiteral("id"), e.id },
            { QStringLiteral("label"), e.label },
        });
    }
    return out;
}

QVariantList ComposerSessionController::modelCatalog() const
{
    return catalogEntries();
}

QStringList ComposerSessionController::models() const
{
    QStringList out;
    const QVariantList entries = catalogEntries();
    out.reserve(entries.size());
    for (const QVariant& v : entries) {
        out << v.toMap().value(QStringLiteral("label")).toString();
    }
    return out;
}

int ComposerSessionController::currentModelIndex() const
{
    if (m_modelSource != nullptr) {
        const QString cur = m_modelSource->currentModelId();
        const QVariantList entries = catalogEntries();
        for (int i = 0; i < entries.size(); ++i) {
            if (entries.at(i).toMap().value(QStringLiteral("id")).toString() == cur) {
                return i;
            }
        }
        return -1;
    }
    return m_currentModelIndex;
}

QString ComposerSessionController::currentModel() const
{
    const QVariantList entries = catalogEntries();
    const int idx = currentModelIndex();
    return (idx >= 0 && idx < entries.size())
        ? entries.at(idx).toMap().value(QStringLiteral("label")).toString()
        : QString();
}

QString ComposerSessionController::currentProvider() const
{
    const QVariantList entries = catalogEntries();
    const int idx = currentModelIndex();
    return (idx >= 0 && idx < entries.size())
        ? entries.at(idx).toMap().value(QStringLiteral("provider")).toString()
        : QString();
}

void ComposerSessionController::setCurrentModelIndex(int index)
{
    const QVariantList entries = catalogEntries();
    if (index < 0 || index >= entries.size()) {
        return;
    }
    if (m_modelSource != nullptr) {
        // Selecting a model in the composer activates it in the shared catalog;
        // currentChanged flows back here and re-emits currentModelChanged.
        const QString id = entries.at(index).toMap().value(QStringLiteral("id")).toString();
        m_modelSource->activate(id);
        return;
    }
    if (index == m_currentModelIndex) {
        return;
    }
    m_currentModelIndex = index;
    emit currentModelChanged();
}

void ComposerSessionController::selectModel(int index)
{
    setCurrentModelIndex(index);
}

QObject* ComposerSessionController::modelSource() const
{
    return m_modelSource.data();
}

void ComposerSessionController::setModelSource(QObject* source)
{
    auto* catalog = qobject_cast<models::IModelCatalog*>(source);
    if (m_modelSource == catalog) {
        return;
    }
    if (m_modelSource != nullptr) {
        m_modelSource->disconnect(this);
        if (auto* prev = qobject_cast<uimodels::VariantListModel*>(m_modelSource->installed())) {
            prev->disconnect(this);
        }
    }
    m_modelSource = catalog;
    if (m_modelSource != nullptr) {
        // The active model changing in the catalog is our current selection.
        connect(m_modelSource, &models::IModelCatalog::currentChanged, this,
                &ComposerSessionController::currentModelChanged);
        // The installed list changing (install/remove) reshapes our model list and
        // can shift the current index, so refresh both.
        if (auto* installed
            = qobject_cast<uimodels::VariantListModel*>(m_modelSource->installed())) {
            const auto refresh = [this] {
                emit modelCatalogChanged();
                emit currentModelChanged();
            };
            connect(installed, &QAbstractItemModel::rowsInserted, this, refresh);
            connect(installed, &QAbstractItemModel::rowsRemoved, this, refresh);
            connect(installed, &QAbstractItemModel::modelReset, this, refresh);
            connect(installed, &QAbstractItemModel::dataChanged, this, refresh);
        }
    }
    emit modelCatalogChanged();
    emit currentModelChanged();
}

void ComposerSessionController::setReasoningEffort(const QString& effort)
{
    // Accept only the known levels; ignore anything else so a bad write can't
    // wedge the segmented control.
    static const QStringList kLevels { QStringLiteral("off"), QStringLiteral("low"),
                                       QStringLiteral("medium"), QStringLiteral("high") };
    if (!kLevels.contains(effort) || m_reasoningEffort == effort) {
        return;
    }
    m_reasoningEffort = effort;
    emit modesChanged();
}

void ComposerSessionController::setFastMode(bool on)
{
    if (m_fastMode == on) {
        return;
    }
    m_fastMode = on;
    emit modesChanged();
}

void ComposerSessionController::setVerbose(bool on)
{
    if (m_verbose == on) {
        return;
    }
    m_verbose = on;
    emit modesChanged();
}

void ComposerSessionController::cycleReasoningEffort()
{
    static const QStringList kLevels { QStringLiteral("off"), QStringLiteral("low"),
                                       QStringLiteral("medium"), QStringLiteral("high") };
    const int i = kLevels.indexOf(m_reasoningEffort);
    setReasoningEffort(kLevels.at((i + 1) % kLevels.size()));
}

void ComposerSessionController::toggleFastMode()
{
    setFastMode(!m_fastMode);
}

void ComposerSessionController::toggleVerbose()
{
    setVerbose(!m_verbose);
}

void ComposerSessionController::resetBrowse()
{
    m_browseIndex = -1;
    m_historyDraft.clear();
}

void ComposerSessionController::pushHistory(const QString& text)
{
    QStringList arr = m_histories.value(m_sessionId);
    if (arr.isEmpty() || arr.last() != text) {
        arr.push_back(text);
    }
    if (arr.size() > 100) {
        arr = arr.mid(arr.size() - 100);
    }
    m_histories[m_sessionId] = arr;
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
    const QStringList arr = m_histories.value(m_sessionId);
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
    const QStringList arr = m_histories.value(m_sessionId);
    if (m_browseIndex < arr.size() - 1) {
        m_browseIndex += 1;
        applyDraft(arr.at(m_browseIndex));
    } else {
        m_browseIndex = -1;
        applyDraft(m_historyDraft);
    }
    return true;
}

void ComposerSessionController::resetReverseSearch()
{
    if (!m_reverseSearching) {
        return;
    }
    m_reverseSearching = false;
    m_reverseFound = true;
    m_reverseIndex = -1;
    m_reverseQuery.clear();
    emit reverseSearchChanged();
}

int ComposerSessionController::reverseSearchResolve(int fromIndex)
{
    const QStringList arr = m_histories.value(m_sessionId);
    for (int i = qMin(fromIndex, static_cast<int>(arr.size()) - 1); i >= 0; --i) {
        if (arr.at(i).contains(m_reverseQuery, Qt::CaseInsensitive)) {
            m_reverseIndex = i;
            m_reverseFound = true;
            applyDraft(arr.at(i)); // preview the hit in both front ends' fields
            return i;
        }
    }
    // No match: keep the current preview in place, but flag the failure so the
    // prompt can render "(failed reverse-i-search)".
    m_reverseFound = false;
    return -1;
}

void ComposerSessionController::reverseSearchStart()
{
    // A second Ctrl+R while already searching steps to the next older match.
    if (m_reverseSearching) {
        reverseSearchNext();
        return;
    }
    m_reverseSavedDraft = m_draft;
    m_reverseSearching = true;
    m_reverseFound = true;
    m_reverseIndex = -1;
    m_reverseQuery.clear();
    // Empty query: no preview yet (the saved draft stays in the field) - the user
    // types to narrow, or presses Ctrl+R again to walk history.
    emit reverseSearchChanged();
}

void ComposerSessionController::reverseSearchNext()
{
    if (!m_reverseSearching) {
        return;
    }
    const QStringList arr = m_histories.value(m_sessionId);
    const int from = (m_reverseIndex == -1) ? static_cast<int>(arr.size()) - 1 : m_reverseIndex - 1;
    reverseSearchResolve(from);
    emit reverseSearchChanged();
}

void ComposerSessionController::reverseSearchType(const QString& chars)
{
    if (!m_reverseSearching || chars.isEmpty()) {
        return;
    }
    m_reverseQuery += chars;
    // Narrowing always re-scans from the newest entry so the most recent match wins.
    reverseSearchResolve(static_cast<int>(m_histories.value(m_sessionId).size()) - 1);
    emit reverseSearchChanged();
}

void ComposerSessionController::reverseSearchBackspace()
{
    if (!m_reverseSearching) {
        return;
    }
    if (m_reverseQuery.isEmpty()) {
        return;
    }
    m_reverseQuery.chop(1);
    if (m_reverseQuery.isEmpty()) {
        // Back to an empty query: revert the preview to the pre-search draft.
        m_reverseIndex = -1;
        m_reverseFound = true;
        applyDraft(m_reverseSavedDraft);
    } else {
        reverseSearchResolve(static_cast<int>(m_histories.value(m_sessionId).size()) - 1);
    }
    emit reverseSearchChanged();
}

void ComposerSessionController::reverseSearchAccept()
{
    if (!m_reverseSearching) {
        return;
    }
    // Keep the previewed match as the editable draft; do NOT submit.
    m_reverseSearching = false;
    m_reverseFound = true;
    m_reverseIndex = -1;
    m_reverseQuery.clear();
    emit reverseSearchChanged();
}

void ComposerSessionController::reverseSearchCancel()
{
    if (!m_reverseSearching) {
        return;
    }
    const QString restoreTo = m_reverseSavedDraft;
    m_reverseSearching = false;
    m_reverseFound = true;
    m_reverseIndex = -1;
    m_reverseQuery.clear();
    applyDraft(restoreTo);
    emit reverseSearchChanged();
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
    resetReverseSearch();
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

void ComposerSessionController::stash(const QString& key)
{
    m_drafts[key] = m_draft;
    m_queues[key] = m_queue->entries();
}

void ComposerSessionController::restore(const QString& key)
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
