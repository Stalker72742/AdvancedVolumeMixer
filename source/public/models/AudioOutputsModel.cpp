//
// Created by Stalker7274 on 05.09.2026.
//

#include "AudioOutputsModel.h"
#include "AudioMixerController.h"

AudioOutputsModel::AudioOutputsModel(AudioMixerController *controller)
    : QAbstractListModel(controller)
    , controller(controller)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &AudioOutputsModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved,  this, &AudioOutputsModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset,   this, &AudioOutputsModel::countChanged);
}

int AudioOutputsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(controller->outputsData().size());
}

QVariant AudioOutputsModel::data(const QModelIndex &index, int role) const
{
    const auto &outputs = controller->outputsData();
    if (!index.isValid() || index.row() >= outputs.size())
        return {};

    const auto &output = outputs.at(index.row());
    switch (role) {
        case IdxRole:           return index.row();
        case IdRole:            return output.OutputId;
        case NameRole:          return output.OutputName;
        case OnlineRole:        return output.Online;
        case IsDefaultRole:     return output.IsDefault;
        case ProfileCountRole:  return static_cast<int>(output.Profiles.size());
        case ActiveProfileNameRole: {
            const int active = output.activeProfileIndex();
            return active >= 0 ? output.Profiles.at(active).Name : QString();
        }
        default:                return {};
    }
}

QHash<int, QByteArray> AudioOutputsModel::roleNames() const
{
    return {
        { IdxRole,               "idx" },
        { IdRole,                "outputId" },
        { NameRole,              "name" },
        { OnlineRole,            "online" },
        { IsDefaultRole,         "isDefault" },
        { ActiveProfileNameRole, "activeProfileName" },
        { ProfileCountRole,      "profileCount" }
    };
}

void AudioOutputsModel::notifyRowChanged(int row)
{
    if (row < 0 || row >= rowCount())
        return;
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
}
