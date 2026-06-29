// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "completion_model.h"
#include "composer_attachment_model.h"
#include "composer_queue_model.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>

namespace models {
class IModelCatalog;
}

// The composer's domain/session state machine, extracted from Composer.qml so it
// is shared verbatim between the QML GUI and the TUI. It owns: the live draft,
// per-session draft/queue stashing, a sent-message history ring, the prompt
// queue (drained when the turn goes idle), in-place queue editing, and the
// pending attachments. It emits exactly the four outbound signals the QML
// composer emitted (submitted/steer/cancelRequested/commandInvoked).
//
// The view still owns everything visual: the text field + caret, the completion
// popover, attachment chips, layout, and key routing. It pushes typed text in via
// `setDraft`, calls the invokables for intents, and listens to `draftReset` to
// replace the field's text (caret-to-end) when the controller changes the draft
// programmatically (history recall, session swap, clear, queue edit).
class ComposerSessionController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
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

    // Model selector. The list + current selection are the single source of truth
    // for the GUI ModelPill/picker and the TUI. When a `modelSource` (IModelCatalog)
    // is injected, the list is the installed models and the current selection is the
    // catalog's active model (so the composer, the Models hub, and Settings ->
    // Model default all agree); otherwise a built-in fallback catalog is used.
    Q_PROPERTY(QStringList models READ models NOTIFY modelCatalogChanged)
    // Structured catalog: one entry per model as {provider,id,label}, in the same
    // flat order as `models` so an index selects the same model in both. The GUI
    // picker overlay groups by provider.
    Q_PROPERTY(QVariantList modelCatalog READ modelCatalog NOTIFY modelCatalogChanged)
    Q_PROPERTY(int currentModelIndex READ currentModelIndex WRITE setCurrentModelIndex NOTIFY
                   currentModelChanged)
    Q_PROPERTY(QString currentModel READ currentModel NOTIFY currentModelChanged)
    Q_PROPERTY(QString currentProvider READ currentProvider NOTIFY currentModelChanged)
    // The live model-catalog seam (IModelCatalog) shared with the Models hub.
    // Settable from QML (modelSource: ModelCatalog) and from the TUI; null = use
    // the built-in fallback catalog.
    Q_PROPERTY(
        QObject* modelSource READ modelSource WRITE setModelSource NOTIFY modelCatalogChanged)

    // Turn modes (client state; no backend yet). reasoningEffort is one of
    // off/low/medium/high; fastMode trades depth for latency; verbose surfaces
    // extra detail. The daemon later reads these off the same controller.
    Q_PROPERTY(
        QString reasoningEffort READ reasoningEffort WRITE setReasoningEffort NOTIFY modesChanged)
    Q_PROPERTY(bool fastMode READ fastMode WRITE setFastMode NOTIFY modesChanged)
    Q_PROPERTY(bool verbose READ verbose WRITE setVerbose NOTIFY modesChanged)

public:
    explicit ComposerSessionController(QObject* parent = nullptr);

    [[nodiscard]] QString sessionId() const { return m_sessionId; }
    void setSessionId(const QString& id);

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

    [[nodiscard]] QStringList models() const;
    [[nodiscard]] QVariantList modelCatalog() const;
    [[nodiscard]] int currentModelIndex() const;
    [[nodiscard]] QString currentModel() const;
    [[nodiscard]] QString currentProvider() const;
    void setCurrentModelIndex(int index);

    [[nodiscard]] QObject* modelSource() const;
    void setModelSource(QObject* source);

    [[nodiscard]] QString reasoningEffort() const { return m_reasoningEffort; }
    Q_INVOKABLE void setReasoningEffort(const QString& effort);
    [[nodiscard]] bool fastMode() const { return m_fastMode; }
    Q_INVOKABLE void setFastMode(bool on);
    [[nodiscard]] bool verbose() const { return m_verbose; }
    Q_INVOKABLE void setVerbose(bool on);

    // Intents (mirror the QML composer's functions).
    Q_INVOKABLE void submit();       // Enter: send / save-edit / drain
    Q_INVOKABLE void enqueueDraft(); // explicit "queue" control
    Q_INVOKABLE void steerDraft();   // Ctrl+Enter mid-turn nudge
    Q_INVOKABLE void cancel();       // Esc/Stop while busy
    Q_INVOKABLE bool browseUp();     // history recall (true = swallow the key)
    Q_INVOKABLE bool browseDown();
    [[nodiscard]] Q_INVOKABLE bool browsing() const { return m_browseIndex != -1; }
    Q_INVOKABLE void sendNowEntry(int index);
    Q_INVOKABLE void deleteEntry(int index);
    Q_INVOKABLE void beginEdit(int index);
    Q_INVOKABLE void exitEdit(bool restore);
    Q_INVOKABLE void addAttachment(const QString& name, const QString& kind);
    Q_INVOKABLE void invokeCommand(const QString& command);
    Q_INVOKABLE void clear();                // clear draft + attachments
    Q_INVOKABLE void selectModel(int index); // pick the active model (clamped)
    // Cycle reasoning effort off->low->medium->high->off (TUI chrome toggle).
    Q_INVOKABLE void cycleReasoningEffort();
    Q_INVOKABLE void toggleFastMode();
    Q_INVOKABLE void toggleVerbose();

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

    void sessionIdChanged();
    void busyChanged();
    void enabledChanged();
    void draftChanged();
    void derivedChanged();
    void queueCountChanged();
    void editingIndexChanged();
    void currentModelChanged();
    void modelCatalogChanged();
    void modesChanged();

private:
    // The unified model list as {provider,id,label} maps - from the injected
    // IModelCatalog's installed models when present, else the fallback catalog.
    [[nodiscard]] QVariantList catalogEntries() const;
    void applyDraft(const QString& text); // programmatic draft change (-> draftReset)
    // Like applyDraft but places the caret at `cursor` (-> draftReset + cursorRequested).
    void applyDraftWithCursor(const QString& text, int cursor);
    void resetBrowse();
    // Clears reverse-search state and notifies (without restoring the draft); used
    // when the context changes out from under an active search (session swap,
    // submit, clear, or an external draft edit).
    void resetReverseSearch();
    // Re-resolve the current query: scan history newest-first from `fromIndex`,
    // preview a hit via applyDraft, and update found/index. Returns the hit index
    // or -1. An empty query matches the newest entry.
    int reverseSearchResolve(int fromIndex);
    void pushHistory(const QString& text);
    void enqueue(const QString& text, const QString& refs);
    void drainNext();
    void stash(const QString& key);
    void restore(const QString& key);
    void setEditingIndex(int index);

    ComposerQueueModel* m_queue = nullptr;
    ComposerAttachmentModel* m_attachments = nullptr;
    CompletionModel* m_completion = nullptr;

    QString m_sessionId;
    bool m_busy = false;
    bool m_enabled = true;
    QString m_draft;

    // Fallback catalog used only when no Models hub source is injected (main app
    // injects MockModelCatalog). Keep ids aligned with the mock catalog so tests
    // and stripped-down hosts do not drift into a second model universe.
    struct ModelEntry {
        QString provider;
        QString id;
        QString label;
    };
    QList<ModelEntry> m_catalog{
        {QStringLiteral("Meta"), QStringLiteral("llama-3.1-8b-instruct"),
         QStringLiteral("Llama 3.1 8B Instruct")},
        {QStringLiteral("Meta"), QStringLiteral("llama-3.1-70b-instruct"),
         QStringLiteral("Llama 3.1 70B Instruct")},
        {QStringLiteral("Mistral"), QStringLiteral("mistral-7b-instruct"),
         QStringLiteral("Mistral 7B Instruct")},
        {QStringLiteral("Mistral"), QStringLiteral("mixtral-8x7b"), QStringLiteral("Mixtral 8x7B")},
        {QStringLiteral("Google"), QStringLiteral("gemma-2-9b-it"),
         QStringLiteral("Gemma 2 9B IT")},
    };
    QStringList m_models;
    int m_currentModelIndex = 0;

    // Optional live catalog seam. When non-null, models()/modelCatalog()/current*
    // are sourced from the installed models + active id here instead of m_catalog.
    QPointer<models::IModelCatalog> m_modelSource;

    // Turn modes (client state).
    QString m_reasoningEffort = QStringLiteral("medium");
    bool m_fastMode = false;
    bool m_verbose = false;

    // Per-session persisted state (keyed by session id).
    QHash<QString, QString> m_drafts;
    QHash<QString, QList<ComposerQueueModel::Entry>> m_queues;
    QHash<QString, QStringList> m_histories;

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
