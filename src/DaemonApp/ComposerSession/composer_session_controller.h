#pragma once

#include "completion_model.h"
#include "composer_attachment_model.h"
#include "composer_queue_model.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

// The composer's domain/session state machine, extracted from Composer.qml so it
// is shared verbatim between the QML GUI and the TUI. It owns: the live draft,
// per-conversation draft/queue stashing, a sent-message history ring, the prompt
// queue (drained when the turn goes idle), in-place queue editing, and the
// pending attachments. It emits exactly the four outbound signals the QML
// composer emitted (submitted/steer/cancelRequested/commandInvoked).
//
// The view still owns everything visual: the text field + caret, the completion
// popover, attachment chips, layout, and key routing. It pushes typed text in via
// `setDraft`, calls the invokables for intents, and listens to `draftReset` to
// replace the field's text (caret-to-end) when the controller changes the draft
// programmatically (history recall, conversation swap, clear, queue edit).
class ComposerSessionController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int conversationId READ conversationId WRITE setConversationId NOTIFY
                   conversationIdChanged)
    Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString draft READ draft WRITE setDraft NOTIFY draftChanged)
    Q_PROPERTY(bool hasPayload READ hasPayload NOTIFY derivedChanged)
    Q_PROPERTY(bool canSteer READ canSteer NOTIFY derivedChanged)
    // The composer's primary action, derived from busy + payload, so the GUI
    // control and the TUI chrome share one rule: "send" (idle), "queue" (busy with
    // a draft), "stop" (busy, empty). `primaryActionEnabled` folds in `enabled`.
    Q_PROPERTY(QString primaryAction READ primaryAction NOTIFY derivedChanged)
    Q_PROPERTY(bool primaryActionEnabled READ primaryActionEnabled NOTIFY derivedChanged)
    Q_PROPERTY(int queueCount READ queueCount NOTIFY queueCountChanged)
    Q_PROPERTY(int editingIndex READ editingIndex NOTIFY editingIndexChanged)
    Q_PROPERTY(ComposerQueueModel* queue READ queue CONSTANT)
    Q_PROPERTY(ComposerAttachmentModel* attachments READ attachments CONSTANT)
    // Completion (slash / @ trigger) state - the view renders these; the FSM lives
    // here so both front ends behave identically.
    Q_PROPERTY(CompletionModel* completionItems READ completionItems CONSTANT)
    Q_PROPERTY(bool completionActive READ completionActive NOTIFY completionActiveChanged)
    Q_PROPERTY(QString completionKind READ completionKind NOTIFY completionKindChanged)
    Q_PROPERTY(
        int completionActiveIndex READ completionActiveIndex NOTIFY completionActiveIndexChanged)

    // Reverse incremental history search (Ctrl+R). The FSM lives here so both
    // front ends share one behavior; the matched entry is previewed into the field
    // via the existing draftReset plumbing, and the view renders the
    // "(reverse-i-search)`query':" prompt from these properties.
    Q_PROPERTY(bool reverseSearching READ reverseSearching NOTIFY reverseSearchChanged)
    Q_PROPERTY(QString reverseSearchQuery READ reverseSearchQuery NOTIFY reverseSearchChanged)
    Q_PROPERTY(bool reverseSearchFound READ reverseSearchFound NOTIFY reverseSearchChanged)

    // Model selector (placeholder: there is no gateway model backend yet, so the
    // selection is in-memory). The list + current index live here so the GUI
    // ModelPill and the TUI share one source.
    Q_PROPERTY(QStringList models READ models CONSTANT)
    Q_PROPERTY(int currentModelIndex READ currentModelIndex WRITE setCurrentModelIndex NOTIFY
                   currentModelChanged)
    Q_PROPERTY(QString currentModel READ currentModel NOTIFY currentModelChanged)

public:
    explicit ComposerSessionController(QObject* parent = nullptr);

    [[nodiscard]] int conversationId() const { return m_conversationId; }
    void setConversationId(int id);

    [[nodiscard]] bool busy() const { return m_busy; }
    void setBusy(bool busy);

    [[nodiscard]] bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    [[nodiscard]] QString draft() const { return m_draft; }
    void setDraft(const QString& draft); // live text from the view (no draftReset)

    [[nodiscard]] bool hasPayload() const;
    [[nodiscard]] bool canSteer() const;
    [[nodiscard]] QString primaryAction() const;
    [[nodiscard]] bool primaryActionEnabled() const;
    [[nodiscard]] int queueCount() const { return m_queue->count(); }
    [[nodiscard]] int editingIndex() const { return m_editingIndex; }

    [[nodiscard]] ComposerQueueModel* queue() const { return m_queue; }
    [[nodiscard]] ComposerAttachmentModel* attachments() const { return m_attachments; }

    [[nodiscard]] CompletionModel* completionItems() const { return m_completion; }
    [[nodiscard]] bool completionActive() const { return m_completionActive; }
    [[nodiscard]] QString completionKind() const { return m_completionKind; }
    [[nodiscard]] int completionActiveIndex() const { return m_completionActiveIndex; }

    [[nodiscard]] bool reverseSearching() const { return m_reverseSearching; }
    [[nodiscard]] QString reverseSearchQuery() const { return m_reverseQuery; }
    [[nodiscard]] bool reverseSearchFound() const { return m_reverseFound; }

    [[nodiscard]] QStringList models() const { return m_models; }
    [[nodiscard]] int currentModelIndex() const { return m_currentModelIndex; }
    [[nodiscard]] QString currentModel() const;
    void setCurrentModelIndex(int index);

    // Intents (mirror the QML composer's functions).
    Q_INVOKABLE void submit();        // Enter: send / save-edit / drain
    Q_INVOKABLE void enqueueDraft();  // explicit "queue" control
    Q_INVOKABLE void steerDraft();    // Ctrl+Enter mid-turn nudge
    Q_INVOKABLE void cancel();        // Esc/Stop while busy
    Q_INVOKABLE bool browseUp();      // history recall (true = swallow the key)
    Q_INVOKABLE bool browseDown();
    [[nodiscard]] Q_INVOKABLE bool browsing() const { return m_browseIndex != -1; }
    Q_INVOKABLE void sendNowEntry(int index);
    Q_INVOKABLE void deleteEntry(int index);
    Q_INVOKABLE void beginEdit(int index);
    Q_INVOKABLE void exitEdit(bool restore);
    Q_INVOKABLE void addAttachment(const QString& name, const QString& kind);
    Q_INVOKABLE void invokeCommand(const QString& command);
    Q_INVOKABLE void clear(); // clear draft + attachments
    Q_INVOKABLE void selectModel(int index); // pick the active model (clamped)

    // Completion FSM (mirror of the QML composer's trigger functions). The view
    // pushes the live text + caret on every change; the controller detects the
    // slash/@ token, filters the pool, and drives active/kind/index. accept()
    // computes the resulting draft + caret (-> draftReset + cursorRequested).
    Q_INVOKABLE void refreshTrigger(const QString& text, int cursorPos);
    Q_INVOKABLE void closeTrigger();
    Q_INVOKABLE void moveActive(int delta);
    Q_INVOKABLE void setActiveIndex(int index);
    Q_INVOKABLE void acceptActive();
    Q_INVOKABLE void accept(int index);

    // Reverse incremental history search (Ctrl+R). start() enters the mode (or
    // steps to the next older match when already active); type()/backspace() narrow
    // or widen the query; next() steps to the next older match; accept() keeps the
    // previewed match in the editable draft (no submit); cancel() restores the
    // pre-search draft. Matching is case-insensitive substring, newest-first.
    Q_INVOKABLE void reverseSearchStart();
    Q_INVOKABLE void reverseSearchNext();
    Q_INVOKABLE void reverseSearchType(const QString& chars);
    Q_INVOKABLE void reverseSearchBackspace();
    Q_INVOKABLE void reverseSearchAccept();
    Q_INVOKABLE void reverseSearchCancel();

signals:
    // Outbound (host-facing) - identical to the QML composer's signals.
    void submitted(const QString& text, const QString& attachmentRefs);
    void steer(const QString& text);
    void cancelRequested();
    void commandInvoked(const QString& command);

    // The controller replaced the draft programmatically; the view should set its
    // text to `text` and move the caret to the end.
    void draftReset(const QString& text);
    // After a completion accept the caret must land at a specific offset (emitted
    // right after draftReset so the view's caret-to-end is overridden).
    void cursorRequested(int position);

    void completionActiveChanged();
    void completionKindChanged();
    void completionActiveIndexChanged();
    void reverseSearchChanged();

    void conversationIdChanged();
    void busyChanged();
    void enabledChanged();
    void draftChanged();
    void derivedChanged();
    void queueCountChanged();
    void editingIndexChanged();
    void currentModelChanged();

private:
    void applyDraft(const QString& text); // programmatic draft change (-> draftReset)
    // Like applyDraft but places the caret at `cursor` (-> draftReset + cursorRequested).
    void applyDraftWithCursor(const QString& text, int cursor);
    void resetBrowse();
    // Clears reverse-search state and notifies (without restoring the draft); used
    // when the context changes out from under an active search (conversation swap,
    // submit, clear, or an external draft edit).
    void resetReverseSearch();
    // Re-resolve the current query: scan history newest-first from `fromIndex`,
    // preview a hit via applyDraft, and update found/index. Returns the hit index
    // or -1. An empty query matches the newest entry.
    int reverseSearchResolve(int fromIndex);
    void pushHistory(const QString& text);
    void enqueue(const QString& text, const QString& refs);
    void drainNext();
    void stash(int key);
    void restore(int key);
    void setEditingIndex(int index);

    ComposerQueueModel* m_queue = nullptr;
    ComposerAttachmentModel* m_attachments = nullptr;
    CompletionModel* m_completion = nullptr;

    int m_conversationId = -1;
    bool m_busy = false;
    bool m_enabled = true;
    QString m_draft;

    // Canned model list (no gateway model backend yet); selection is in-memory.
    QStringList m_models { QStringLiteral("claude-opus-4.8"), QStringLiteral("claude-sonnet-4.6"),
                           QStringLiteral("gpt-5.5"), QStringLiteral("gpt-5.3-codex"),
                           QStringLiteral("gemini-3-pro") };
    int m_currentModelIndex = 0;

    // Per-conversation persisted state (keyed by conversation id).
    QHash<int, QString> m_drafts;
    QHash<int, QList<ComposerQueueModel::Entry>> m_queues;
    QHash<int, QStringList> m_histories;

    int m_browseIndex = -1;
    QString m_historyDraft;

    // Reverse incremental search (Ctrl+R) state. m_reverseIndex is the history row
    // currently previewed (-1 = none); m_reverseSavedDraft is restored on cancel.
    bool m_reverseSearching = false;
    bool m_reverseFound = true;
    int m_reverseIndex = -1;
    QString m_reverseQuery;
    QString m_reverseSavedDraft;

    int m_editingIndex = -1;
    QString m_preEditDraft;

    bool m_draining = false;

    // Completion FSM state. m_triggerStart/m_triggerEnd bracket the typed token
    // ([trigger char .. caret)) in the draft, captured on the last refresh.
    bool m_completionActive = false;
    QString m_completionKind = QStringLiteral("slash");
    int m_completionActiveIndex = 0;
    int m_triggerStart = -1;
    int m_triggerEnd = -1;
};
