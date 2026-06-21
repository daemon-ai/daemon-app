#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>

// The composer's pending attachment chips. Each entry is a { name, kind } pair
// where kind is one of "file" / "folder" / "image" / "url"; `buildRefs` renders
// them to the space-joined @file:/@folder:/@image:/@url: string a turn carries.
// GUI-free: consumed as a QML model and directly from C++.
class ComposerAttachmentModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    struct Attachment {
        QString name;
        QString kind;
    };

    enum Role {
        NameRole = Qt::UserRole + 1,
        KindRole,
    };

    explicit ComposerAttachmentModel(QObject* parent = nullptr);

    [[nodiscard]] int count() const { return static_cast<int>(m_items.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void append(const QString& name, const QString& kind);
    Q_INVOKABLE void removeAt(int index);
    void clear();

    // The space-joined reference string for the current attachments ("" if none).
    [[nodiscard]] QString buildRefs() const;

signals:
    void countChanged();

private:
    QList<Attachment> m_items;
};
