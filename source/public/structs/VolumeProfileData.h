//
// Created by Stalker7274 on 05.09.2026.
//

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QUuid>

struct ProcessRuleData
{
    QString ProcessName;
    QString Guid;
    QString Scope = QStringLiteral("single");
    double  Volume = 1.0;
    bool    Muted = false;

    // Runtime only, not persisted: set by the session watcher once the
    // process actually opens an audio session on this output.
    bool    SessionActive = false;

    // "group" rules hold several names separated by ';' or ','
    [[nodiscard]] QStringList processNames() const
    {
        QStringList names;
        for (const auto& part : ProcessName.split(QRegularExpression(QStringLiteral("[;,]")), Qt::SkipEmptyParts)) {
            const QString name = part.trimmed();
            if (!name.isEmpty())
                names.append(name);
        }
        return names;
    }

    // Compares executable names case-insensitively, ".exe" is optional on both sides
    static QString normalizedProcessName(const QString& name)
    {
        QString result = name.trimmed().toLower();
        if (result.endsWith(QStringLiteral(".exe")))
            result.chop(4);
        return result;
    }

    [[nodiscard]] bool matches(const QString& exeName) const
    {
        if (Scope == QStringLiteral("all"))
            return true;
        const QString target = normalizedProcessName(exeName);
        for (const auto& name : processNames()) {
            if (normalizedProcessName(name) == target)
                return true;
        }
        return false;
    }

    // Single > group > all, so a specific rule always wins over a broad one
    [[nodiscard]] int priority() const
    {
        if (Scope == QStringLiteral("all"))   return 0;
        if (Scope == QStringLiteral("group")) return 1;
        return 2;
    }

    [[nodiscard]] QJsonObject toJson() const
    {
        return {
            { "processName", ProcessName },
            { "guid",        Guid },
            { "scope",       Scope },
            { "volume",      Volume },
            { "muted",       Muted }
        };
    }

    static ProcessRuleData fromJson(const QJsonObject& obj)
    {
        ProcessRuleData rule;
        rule.ProcessName = obj.value("processName").toString();
        rule.Guid        = obj.value("guid").toString();
        rule.Scope       = obj.value("scope").toString(QStringLiteral("single"));
        rule.Volume      = qBound(0.0, obj.value("volume").toDouble(1.0), 1.0);
        rule.Muted       = obj.value("muted").toBool(false);
        return rule;
    }
};

struct VolumeProfileData
{
    QString                Id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString                Name;
    QStringList            Hotkey;
    QList<ProcessRuleData> Rules;

    [[nodiscard]] QJsonObject toJson() const
    {
        QJsonArray rules;
        for (const auto& rule : Rules)
            rules.append(rule.toJson());

        return {
            { "id",     Id },
            { "name",   Name },
            { "hotkey", QJsonArray::fromStringList(Hotkey) },
            { "rules",  rules }
        };
    }

    static VolumeProfileData fromJson(const QJsonObject& obj)
    {
        VolumeProfileData profile;
        const QString id = obj.value("id").toString();
        if (!id.isEmpty())
            profile.Id = id;
        profile.Name = obj.value("name").toString();

        for (const auto& key : obj.value("hotkey").toArray())
            profile.Hotkey.append(key.toString());

        for (const auto& rule : obj.value("rules").toArray())
            profile.Rules.append(ProcessRuleData::fromJson(rule.toObject()));

        return profile;
    }

    // Best rule for a running executable, nullptr when nothing matches
    [[nodiscard]] const ProcessRuleData* ruleFor(const QString& exeName) const
    {
        const ProcessRuleData* best = nullptr;
        for (const auto& rule : Rules) {
            if (rule.matches(exeName) && (!best || rule.priority() > best->priority()))
                best = &rule;
        }
        return best;
    }
};
