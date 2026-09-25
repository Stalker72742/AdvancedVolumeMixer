//
// Created by Stalker7274 on 05.09.2026.
//

#include "MixerProfileModel.h"
#include "AudioMixerController.h"

MixerProfileModel::MixerProfileModel(AudioMixerController *controller)
    : QAbstractListModel(controller)
    , controller(controller)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &MixerProfileModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved,  this, &MixerProfileModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset,   this, &MixerProfileModel::countChanged);
}

int MixerProfileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    const auto *output = controller->currentOutputData();
    return output ? static_cast<int>(output->Profiles.size()) : 0;
}

QVariant MixerProfileModel::data(const QModelIndex &index, int role) const
{
    const auto *output = controller->currentOutputData();
    if (!output || !index.isValid() || index.row() >= output->Profiles.size())
        return {};

    const auto &profile = output->Profiles.at(index.row());
    switch (role) {
        case IdxRole:       return index.row();
        case ProfileIdRole: return profile.Id;
        case NameRole:      return profile.Name;
        case IsActiveRole:  return profile.Id == output->ActiveProfileId;
        case RuleCountRole: return static_cast<int>(profile.Rules.size());
        case HotkeyRole:    return profile.Hotkey.join(QStringLiteral(" + "));
        default:            return {};
    }
}

QHash<int, QByteArray> MixerProfileModel::roleNames() const
{
    return {
        { IdxRole,       "idx" },
        { ProfileIdRole, "profileId" },
        { NameRole,      "name" },
        { IsActiveRole,  "isActive" },
        { RuleCountRole, "ruleCount" },
        { HotkeyRole,    "hotkey" }
    };
}

void MixerProfileModel::notifyRowChanged(int row)
{
    if (row < 0 || row >= rowCount())
        return;
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
}

void MixerProfileModel::notifyAllChanged()
{
    if (rowCount() == 0)
        return;
    emit dataChanged(index(0), index(rowCount() - 1));
}
