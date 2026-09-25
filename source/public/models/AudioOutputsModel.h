//
// Created by Stalker7274 on 05.09.2026.
//

#pragma once

#include <QAbstractListModel>
#include <qqmlintegration.h>

class AudioMixerController;

// Read-only view over AudioMixerController's outputs. All mutations go
// through the controller, which drives the notifications.
//
// Filtered: shows either the regular outputs or the removed ones
// (Mixer.showRemoved). The "idx" role is the index in the controller's
// list, that's what Mixer.selectOutput()/removeOutput()/... take.
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
        RemovedRole,
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

    // Recomputes which outputs are shown; full reset, the list is tiny
    void rebuild();
    // sourceIndex = index in the controller's list
    void notifyRowChanged(int sourceIndex);

    AudioMixerController* controller = nullptr;
    QList<int> rows; // row -> controller index
};
