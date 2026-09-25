//
// Created by Stalker7274 on 05.09.2026.
//

#pragma once

#include <QAbstractListModel>
#include <qqmlintegration.h>

class AudioMixerController;

// Read-only view over AudioMixerController's outputs. All mutations go
// through the controller, which drives begin/end* notifications.
class AudioOutputsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by Mixer")
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:

    explicit AudioOutputsModel(AudioMixerController *controller);

    enum Roles {
        IdxRole = Qt::UserRole + 1,
        IdRole,
        NameRole,
        OnlineRole,
        IsDefaultRole,
        ActiveProfileNameRole,
        ProfileCountRole
    };
    Q_ENUM(Roles)

signals:
    void countChanged();

public:

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int count() const { return rowCount(); }

protected:

    friend class AudioMixerController;

    void notifyRowChanged(int row);

    AudioMixerController* controller = nullptr;
};
