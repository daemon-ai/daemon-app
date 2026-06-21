#pragma once

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
    Q_PROPERTY(int queueCount READ queueCount NOTIFY queueCountChanged)
    Q_PROPERTY(int editingIndex READ editingIndex NOTIFY editingIndexChanged)
    Q_PROPERTY(ComposerQueueModel* queue READ queue CONSTANT)
    Q_PROPERTY(ComposerAttachmentModel* attachments READ attachments CONSTANT)

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
    [[nodiscard]] int queueCount() const { return m_queue->count(); }
    [[nodiscard]] int editingIndex() const { return m_editingIndex; }

    [[nodiscard]] ComposerQueueModel* queue() const { return m_queue; }
    [[nodiscard]] ComposerAttachmentModel* attachments() const { return m_attachments; }

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

signals:
    // Outbound (host-facing) - identical to the QML composer's signals.
    void submitted(const QString& text, const QString& attachmentRefs);
    void steer(const QString& text);
    void cancelRequested();
    void commandInvoked(const QString& command);

    // The controller replaced the draft programmatically; the view should set its
    // text to `text` and move the caret to the end.
    void draftReset(const QString& text);

    void conversationIdChanged();
    void busyChanged();
    void enabledChanged();
    void draftChanged();
    void derivedChanged();
    void queueCountChanged();
    void editingIndexChanged();

private:
    void applyDraft(const QString& text); // programmatic draft change (-> draftReset)
    void resetBrowse();
    void pushHistory(const QString& text);
    void enqueue(const QString& text, const QString& refs);
    void drainNext();
    void stash(int key);
    void restore(int key);
    void setEditingIndex(int index);

    ComposerQueueModel* m_queue = nullptr;
    ComposerAttachmentModel* m_attachments = nullptr;

    int m_conversationId = -1;
    bool m_busy = false;
    bool m_enabled = true;
    QString m_draft;

    // Per-conversation persisted state (keyed by conversation id).
    QHash<int, QString> m_drafts;
    QHash<int, QList<ComposerQueueModel::Entry>> m_queues;
    QHash<int, QStringList> m_histories;

    int m_browseIndex = -1;
    QString m_historyDraft;

    int m_editingIndex = -1;
    QString m_preEditDraft;

    bool m_draining = false;
};
