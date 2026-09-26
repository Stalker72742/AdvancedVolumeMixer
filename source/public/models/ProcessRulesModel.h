//
// Created by Stalker7274 on 25.09.2026.
//

#pragma once

#include <QAbstractListModel>
#include <qqmlintegration.h>

class AudioMixerController;

// Process rules of the profile currently viewed in AudioMixerController.
class ProcessRulesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by Mixer")
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:

    explicit ProcessRulesModel(AudioMixerController *controller);

    enum Roles {
        IdxRole = Qt::UserRole + 1,
        ProcessNameRole,
        GuidRole,
        ScopeRole,
        VolumeRole,
        MutedRole,
        SessionActiveRole
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

    // Called by the controller after it changed the data
    void notifyRowChanged(int row, const QList<int> &roles = {});
    void notifyAllChanged(const QList<int> &roles = {});

    AudioMixerController* controller = nullptr;
};
