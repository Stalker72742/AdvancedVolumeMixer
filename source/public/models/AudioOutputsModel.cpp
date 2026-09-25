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
    return static_cast<int>(rows.size());
}

QVariant AudioOutputsModel::data(const QModelIndex &index, int role) const
{
    const auto &outputs = controller->outputsData();
    if (!index.isValid() || index.row() >= rows.size())
        return {};

    const int source = rows.at(index.row());
    if (source < 0 || source >= outputs.size())
        return {};

    const auto &output = outputs.at(source);
    switch (role) {
        case IdxRole:           return source;
        case IdRole:            return output.OutputId;
        case NameRole:          return output.OutputName;
        case OnlineRole:        return output.Online;
        case IsDefaultRole:     return output.IsDefault;
        case RemovedRole:       return output.Removed;
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
        { RemovedRole,           "removed" },
        { ActiveProfileNameRole, "activeProfileName" },
        { ProfileCountRole,      "profileCount" }
    };
}

void AudioOutputsModel::rebuild()
{
    beginResetModel();
    rows.clear();
    const auto &outputs = controller->outputsData();
    const bool showRemoved = controller->showRemoved();
    for (int i = 0; i < outputs.size(); ++i) {
        if (outputs.at(i).Removed == showRemoved)
            rows.append(i);
    }
    endResetModel();
}

void AudioOutputsModel::notifyRowChanged(int sourceIndex)
{
    const int row = static_cast<int>(rows.indexOf(sourceIndex));
    if (row < 0)
        return;
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
}
