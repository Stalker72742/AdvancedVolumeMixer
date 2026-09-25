//
// Created by Stalker7274 on 05.09.2026.
//

#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include "structs/VolumeProfileData.h"

struct AudioOutputData
{
    QString                  OutputId;       // WASAPI endpoint ID, stable between reboots
    QString                  OutputName;
    QString                  ActiveProfileId;
    QList<VolumeProfileData> Profiles;
    QDateTime                LastSeen;

    // Runtime only, not persisted
    bool Online    = false;
    bool IsDefault = false;

    [[nodiscard]] int activeProfileIndex() const
    {
        for (int i = 0; i < Profiles.size(); ++i) {
            if (Profiles.at(i).Id == ActiveProfileId)
                return i;
        }
        return Profiles.isEmpty() ? -1 : 0;
    }

    [[nodiscard]] QJsonObject toJson() const
    {
        QJsonArray profiles;
        for (const auto& profile : Profiles)
            profiles.append(profile.toJson());

        return {
            { "id",              OutputId },
            { "name",            OutputName },
            { "activeProfileId", ActiveProfileId },
            { "lastSeen",        LastSeen.toString(Qt::ISODate) },
            { "profiles",        profiles }
        };
    }

    static AudioOutputData fromJson(const QJsonObject& obj)
    {
        AudioOutputData output;
        output.OutputId        = obj.value("id").toString();
        output.OutputName      = obj.value("name").toString();
        output.ActiveProfileId = obj.value("activeProfileId").toString();
        output.LastSeen        = QDateTime::fromString(obj.value("lastSeen").toString(), Qt::ISODate);

        for (const auto& profile : obj.value("profiles").toArray())
            output.Profiles.append(VolumeProfileData::fromJson(profile.toObject()));

        return output;
    }
};
