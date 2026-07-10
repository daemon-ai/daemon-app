// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>

class ComposerQueueStorage;

// The prompt queue backing the composer's QueuePanel. A flat list of
// { text, refs } entries; `refs` is the space-joined @file:/@image: string
// captured when the entry was queued. GUI-free (no QtQuick): it is consumed both
// as a QML model (role names below) and directly from C++ in the TUI.
//
// Storage is pluggable (spec 09 §6.7 step 1): by default the entries live in the in-memory
// `QList<Entry>` below, exactly as before. When a `ComposerQueueStorage` is attached (the durable
// outbox turn-prompt lane), the SAME roles and signals are emitted while the entries are persisted
// through it — consumers (QML + TUI) cannot tell the difference; queued prompts merely gain
// crash-durability. The default (no storage) path is unchanged, so the existing composer tests are
// the untouched regression harness.
class ComposerQueueModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    struct Entry {
        QString text;
        QString refs;
    };

    enum Role {
        TextRole = Qt::UserRole + 1,
        RefsRole,
    };

    explicit ComposerQueueModel(QObject* parent = nullptr);

    [[nodiscard]] int count() const { return static_cast<int>(m_entries.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void append(const QString& text, const QString& refs);
    void setEntry(int index, const QString& text, const QString& refs);
    void insertAt(int index, const QString& text, const QString& refs);
    void removeAt(int index);
    void clear();

    [[nodiscard]] QString textAt(int index) const;
    [[nodiscard]] QString refsAt(int index) const;

    // Bulk swap for per-session stash/restore.
    [[nodiscard]] QList<Entry> entries() const { return m_entries; }
    void setEntries(const QList<Entry>& entries);

    // Attach (or detach with nullptr) a durable storage backend (spec 09 §6.7). Non-null hydrates
    // the model from the backend once (a single reset — initial population only), then every
    // subsequent mutation is persisted through it. The model's rows/roles/signals are unchanged;
    // storage is orthogonal to the presented data. Ownership stays with the caller.
    void setStorage(ComposerQueueStorage* storage);

signals:
    void countChanged();

private:
    // Push the current entries to the attached storage (no-op when none). Called after every
    // mutation so the durable copy tracks the model.
    void persist();

    QList<Entry> m_entries;
    ComposerQueueStorage* m_storage = nullptr;
};

// The durable-storage seam for the composer queue (spec 09 §6.7). Implemented by the outbox
// turn-prompt lane adapter (`mirror::OutboxComposerQueueStorage`) in the local-db module; kept as
// an abstract interface here so the composer module gains no dependency on the outbox in the
// default (in-memory) build.
class ComposerQueueStorage {
public:
    ComposerQueueStorage() = default;
    virtual ~ComposerQueueStorage() = default;
    ComposerQueueStorage(const ComposerQueueStorage&) = delete;
    ComposerQueueStorage& operator=(const ComposerQueueStorage&) = delete;

    // FIFO-ordered entries currently persisted for this queue's scope.
    [[nodiscard]] virtual QList<ComposerQueueModel::Entry> loadEntries() const = 0;
    // Replace the persisted entries with `entries` (FIFO order preserved).
    virtual void persistEntries(const QList<ComposerQueueModel::Entry>& entries) = 0;
};
