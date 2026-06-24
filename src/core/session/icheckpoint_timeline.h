#pragma once

#include <QObject>
#include <QString>

namespace session {

// Checkpoint/rewind timeline seam backing the composer's checkpoints panel. Rows
// (VariantListModel): id, label, time, tokens, current. restore() rewinds the
// session to a checkpoint (the mock flips `current` and drops later entries).
class ICheckpointTimeline : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* checkpoints READ checkpoints CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY changed)
    // The conversation whose timeline is shown. Setting it swaps the live
    // checkpoints to that conversation's own timeline (per-conversation rewind),
    // emitting changed() so the bound panel refreshes.
    Q_PROPERTY(int sessionId READ sessionId WRITE setConversationId NOTIFY
                   conversationIdChanged)

public:
    using QObject::QObject;
    ~ICheckpointTimeline() override = default;

    [[nodiscard]] virtual QObject* checkpoints() const = 0;
    [[nodiscard]] virtual int count() const = 0;
    [[nodiscard]] virtual int sessionId() const = 0;
    virtual void setConversationId(int id) = 0;

    Q_INVOKABLE virtual void restore(const QString& id) = 0;
    Q_INVOKABLE virtual QString createCheckpoint(const QString& label) = 0;

signals:
    void changed();
    void conversationIdChanged();
};

} // namespace session
