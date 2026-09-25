//
// Created by Stalker7274 on 25.09.2026.
//

#include "ProcessRulesModel.h"
#include "AudioMixerController.h"

ProcessRulesModel::ProcessRulesModel(AudioMixerController *controller)
    : QAbstractListModel(controller)
    , controller(controller)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &ProcessRulesModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved,  this, &ProcessRulesModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset,   this, &ProcessRulesModel::countChanged);
}

int ProcessRulesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    const auto *profile = controller->currentProfileData();
    return profile ? static_cast<int>(profile->Rules.size()) : 0;
}

QVariant ProcessRulesModel::data(const QModelIndex &index, int role) const
{
    const auto *profile = controller->currentProfileData();
    if (!profile || !index.isValid() || index.row() >= profile->Rules.size())
        return {};

    const auto &rule = profile->Rules.at(index.row());
    switch (role) {
        case IdxRole:           return index.row();
        case ProcessNameRole:   return rule.ProcessName;
        case GuidRole:          return rule.Guid;
        case ScopeRole:         return rule.Scope;
        case VolumeRole:        return rule.Volume;
        case MutedRole:         return rule.Muted;
        case SessionActiveRole: return rule.SessionActive;
        default:                return {};
    }
}

QHash<int, QByteArray> ProcessRulesModel::roleNames() const
{
    return {
        { IdxRole,           "idx" },
        { ProcessNameRole,   "processName" },
        { GuidRole,          "guid" },
        { ScopeRole,         "scope" },
        { VolumeRole,        "volume" },
        { MutedRole,         "muted" },
        { SessionActiveRole, "sessionActive" }
    };
}

void ProcessRulesModel::notifyRowChanged(int row, const QList<int> &roles)
{
    if (row < 0 || row >= rowCount())
        return;
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, roles);
}

void ProcessRulesModel::notifyAllChanged(const QList<int> &roles)
{
    if (rowCount() == 0)
        return;
    emit dataChanged(index(0), index(rowCount() - 1), roles);
}
