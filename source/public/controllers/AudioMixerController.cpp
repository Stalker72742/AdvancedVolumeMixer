//
// Created by Stalker7274 on 25.09.2026.
//

#include "AudioMixerController.h"
#include "AudioDeviceWatcher.h"
#include "AudioSessionManager.h"
#include "GlobalHotkeyManager.h"

#include <algorithm>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>

namespace
{
    constexpr int ConfigVersion = 1;
    constexpr int SaveDelayMs = 400;
}

AudioMixerController::AudioMixerController(QObject *parent)
    : QObject(parent)
    , watcher(new AudioDeviceWatcher(this))
    , sessions(new AudioSessionManager(this))
    , hotkeys(new GlobalHotkeyManager(this))
    , outputsModel(new AudioOutputsModel(this))
    , profilesModel(new MixerProfileModel(this))
    , rulesModel(new ProcessRulesModel(this))
{
    saveTimer.setSingleShot(true);
    saveTimer.setInterval(SaveDelayMs);
    connect(&saveTimer, &QTimer::timeout, this, &AudioMixerController::saveConfig);

    connect(watcher, &AudioDeviceWatcher::endpointsChanged, this, &AudioMixerController::syncEndpoints);
    connect(watcher, &AudioDeviceWatcher::defaultOutputChanged, this, &AudioMixerController::syncEndpoints);

    connect(sessions, &AudioSessionManager::sessionsChanged, this, &AudioMixerController::onSessionsChanged);
    connect(sessions, &AudioSessionManager::sessionsAppeared, this, &AudioMixerController::onSessionsAppeared);

    connect(hotkeys, &GlobalHotkeyManager::activated, this, &AudioMixerController::onHotkeyActivated);
    connect(this, &AudioMixerController::hotkeysChanged, this, &AudioMixerController::registerHotkeys);

    loadConfig();
    syncEndpoints();

    setCurrentOutputInternal(firstVisibleOutput());

    registerHotkeys();
}

AudioMixerController::~AudioMixerController()
{
    // In edit mode saveConfig() writes the committed snapshot, so
    // unaccepted edits are discarded on exit.
    if (saveTimer.isActive() || isEditMode)
        saveConfig();
}

// ---- Getters ----

QString AudioMixerController::currentOutputName() const
{
    const auto *output = currentOutputData();
    return output ? output->OutputName : QString();
}

bool AudioMixerController::currentOutputOnline() const
{
    const auto *output = currentOutputData();
    return output && output->Online;
}

QString AudioMixerController::currentProfileName() const
{
    const auto *profile = currentProfileData();
    return profile ? profile->Name : QString();
}

QStringList AudioMixerController::currentProfileHotkey() const
{
    const auto *profile = currentProfileData();
    return profile ? profile->Hotkey : QStringList();
}

QString AudioMixerController::currentProfileHotkeyError() const
{
    const auto *output = currentOutputData();
    const auto *profile = currentProfileData();
    if (isEditMode || !output || !profile || profile->Hotkey.isEmpty())
        return {};
    return failedHotkeys.contains(output->OutputId + QLatin1Char('|') + profile->Id)
           ? QStringLiteral("Can't register: taken by another app/profile or unsupported combo")
           : QString();
}

int AudioMixerController::activeProfileIndex() const
{
    const auto *output = currentOutputData();
    return output ? output->activeProfileIndex() : -1;
}

QString AudioMixerController::configPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/profiles.json");
}

const AudioOutputData* AudioMixerController::currentOutputData() const
{
    if (currentOutput < 0 || currentOutput >= outputsList.size())
        return nullptr;
    return &outputsList.at(currentOutput);
}

const VolumeProfileData* AudioMixerController::currentProfileData() const
{
    const auto *output = currentOutputData();
    if (!output || currentProfile < 0 || currentProfile >= output->Profiles.size())
        return nullptr;
    return &output->Profiles.at(currentProfile);
}

AudioOutputData* AudioMixerController::mutableCurrentOutput()
{
    if (currentOutput < 0 || currentOutput >= outputsList.size())
        return nullptr;
    return &outputsList[currentOutput];
}

VolumeProfileData* AudioMixerController::mutableCurrentProfile()
{
    auto *output = mutableCurrentOutput();
    if (!output || currentProfile < 0 || currentProfile >= output->Profiles.size())
        return nullptr;
    return &output->Profiles[currentProfile];
}

// ---- Outputs ----

void AudioMixerController::selectOutput(int index)
{
    if (index < 0 || index >= outputsList.size() || index == currentOutput || outputsList.at(index).Removed)
        return;
    setCurrentOutputInternal(index);
}

void AudioMixerController::removeOutput(int index)
{
    if (isEditMode || index < 0 || index >= outputsList.size() || outputsList.at(index).Removed)
        return;

    const QString outputId = outputsList.at(index).OutputId;

    // Apps we changed on this output go back to 100% before it stops being watched
    sessions->applyProfile(outputId, VolumeProfileData{});

    outputsList[index].Removed = true;
    refreshOutputs();
    updateWatchedSessions();

    if (index == currentOutput)
        setCurrentOutputInternal(firstVisibleOutput());

    emit outputRemoved(outputId);
    emit hotkeysChanged();
    scheduleSave();
}

void AudioMixerController::restoreOutput(int index)
{
    if (isEditMode || index < 0 || index >= outputsList.size() || !outputsList.at(index).Removed)
        return;

    const QString outputId = outputsList.at(index).OutputId;
    outputsList[index].Removed = false;

    // Nothing left to show in the removed list: back to the regular one
    if (removedCount() == 0 && isShowingRemoved) {
        isShowingRemoved = false;
        emit showRemovedChanged();
    }

    refreshOutputs();
    updateWatchedSessions();

    if (currentOutput < 0)
        setCurrentOutputInternal(index);
    if (outputsList.at(index).Online)
        applyActiveProfile(outputId);

    emit outputRestored(outputId);
    emit hotkeysChanged();
    scheduleSave();
}

void AudioMixerController::forgetOutput(int index)
{
    if (isEditMode || index < 0 || index >= outputsList.size())
        return;
    // Only removed + disconnected: an online device would pop right back as new
    const auto &output = outputsList.at(index);
    if (!output.Removed || output.Online)
        return;

    const QString outputId = output.OutputId;
    const QString selectedId = currentOutputData() ? currentOutputData()->OutputId : QString();

    outputsList.removeAt(index);
    if (removedCount() == 0 && isShowingRemoved) {
        isShowingRemoved = false;
        emit showRemovedChanged();
    }
    refreshOutputs();

    // Indexes after the removed one shifted
    const int selected = indexOfOutput(selectedId);
    setCurrentOutputInternal(selected >= 0 ? selected : firstVisibleOutput());

    emit outputForgotten(outputId);
    scheduleSave();
}

void AudioMixerController::setShowRemoved(bool show)
{
    if (isShowingRemoved == show)
        return;
    isShowingRemoved = show;
    outputsModel->rebuild();
    emit showRemovedChanged();
}

int AudioMixerController::removedCount() const
{
    return static_cast<int>(std::count_if(outputsList.cbegin(), outputsList.cend(),
                                          [](const AudioOutputData &o) { return o.Removed; }));
}

void AudioMixerController::refreshOutputs()
{
    outputsModel->rebuild();
    emit removedCountChanged();
}

void AudioMixerController::updateWatchedSessions()
{
    QStringList watchedIds;
    for (const auto &output : std::as_const(outputsList)) {
        if (output.Online && !output.Removed)
            watchedIds.append(output.OutputId);
    }
    sessions->setWatchedDevices(watchedIds);
}

int AudioMixerController::firstVisibleOutput() const
{
    int result = -1;
    for (int i = 0; i < outputsList.size(); ++i) {
        const auto &output = outputsList.at(i);
        if (output.Removed)
            continue;
        if (output.IsDefault)
            return i;
        if (result < 0 || (output.Online && !outputsList.at(result).Online))
            result = i;
    }
    return result;
}

void AudioMixerController::setCurrentOutputInternal(int index)
{
    currentOutput = index;
    const auto *output = currentOutputData();
    currentProfile = output ? output->activeProfileIndex() : -1;
    resetProfileModels();

    emit currentOutputChanged();
    emit currentProfileChanged();
    emit activeProfileChanged();
}

void AudioMixerController::syncEndpoints()
{
    const auto endpoints = watcher->enumerateOutputs();
    const QDateTime now = QDateTime::currentDateTime();

    QSet<QString> seen;
    bool needsSave = false;
    bool structureChanged = false;

    for (const auto &endpoint : endpoints) {
        seen.insert(endpoint.id);
        const int idx = indexOfOutput(endpoint.id);

        if (idx < 0) {
            AudioOutputData output;
            output.OutputId   = endpoint.id;
            output.OutputName = endpoint.name;
            output.LastSeen   = now;
            output.Online     = true;
            output.IsDefault  = endpoint.isDefault;
            output.Profiles.append(makeDefaultProfile());
            output.ActiveProfileId = output.Profiles.first().Id;

            outputsList.append(output);
            structureChanged = true;

            qDebug() << "New audio output:" << endpoint.name;
            emit outputAdded(endpoint.id, endpoint.name);
            if (!isEditMode)
                applyActiveProfile(endpoint.id);
            needsSave = true;
            continue;
        }

        const auto &current = outputsList.at(idx);
        const bool wasOnline = current.Online;
        const bool changed = !wasOnline
                             || current.IsDefault != endpoint.isDefault
                             || (!endpoint.name.isEmpty() && current.OutputName != endpoint.name);
        if (!changed)
            continue;

        auto &output = outputsList[idx];
        if (!endpoint.name.isEmpty() && output.OutputName != endpoint.name) {
            output.OutputName = endpoint.name;
            needsSave = true;
        }
        output.Online    = true;
        output.IsDefault = endpoint.isDefault;
        output.LastSeen  = now;
        outputsModel->notifyRowChanged(idx);

        if (!wasOnline) {
            emit outputConnected(output.OutputId);
            if (!isEditMode && !output.Removed)
                applyActiveProfile(output.OutputId);
        }
    }

    for (int i = 0; i < outputsList.size(); ++i) {
        const auto &current = outputsList.at(i);
        if (seen.contains(current.OutputId) || (!current.Online && !current.IsDefault))
            continue;

        auto &output = outputsList[i];
        const bool wasOnline = output.Online;
        output.Online    = false;
        output.IsDefault = false;
        outputsModel->notifyRowChanged(i);

        if (wasOnline)
            emit outputDisconnected(output.OutputId);
    }

    if (structureChanged)
        refreshOutputs();

    if (currentOutput < 0)
        setCurrentOutputInternal(firstVisibleOutput());
    else
        emit currentOutputChanged();

    if (needsSave)
        scheduleSave();

    updateWatchedSessions();
}

// ---- Profiles ----

void AudioMixerController::selectProfile(int index)
{
    const auto *output = currentOutputData();
    if (!output || index < 0 || index >= output->Profiles.size())
        return;

    if (isEditMode) {
        // Edit mode only navigates, the active profile stays untouched
        if (index == currentProfile)
            return;
        currentProfile = index;
        rulesModel->beginResetModel();
        rulesModel->endResetModel();
        emit currentProfileChanged();
        return;
    }

    activateProfile(currentOutput, index);
}

void AudioMixerController::activateProfile(int outputIndex, int profileIndex)
{
    if (isEditMode || outputIndex < 0 || outputIndex >= outputsList.size())
        return;
    const auto &output = outputsList.at(outputIndex);
    if (profileIndex < 0 || profileIndex >= output.Profiles.size())
        return;

    const QString profileId = output.Profiles.at(profileIndex).Id;
    const bool activeChanged = output.ActiveProfileId != profileId;
    if (activeChanged)
        outputsList[outputIndex].ActiveProfileId = profileId;

    if (outputIndex == currentOutput && profileIndex != currentProfile) {
        currentProfile = profileIndex;
        rulesModel->beginResetModel();
        rulesModel->endResetModel();
        emit currentProfileChanged();
    }

    if (!activeChanged)
        return;

    outputsModel->notifyRowChanged(outputIndex);
    if (outputIndex == currentOutput) {
        profilesModel->notifyAllChanged();
        emit activeProfileChanged();
    }
    applyActiveProfile(outputsList.at(outputIndex).OutputId);
    scheduleSave();
}

void AudioMixerController::addProfile()
{
    if (!currentOutputData())
        return;

    // Creating a profile is an edit: Cancel removes it again
    beginEdit();

    auto *output = mutableCurrentOutput();
    VolumeProfileData profile;
    profile.Name = uniqueProfileName(*output);

    const int row = static_cast<int>(output->Profiles.size());
    profilesModel->beginInsertRows({}, row, row);
    output->Profiles.append(profile);
    profilesModel->endInsertRows();

    currentProfile = row;
    rulesModel->beginResetModel();
    rulesModel->endResetModel();
    outputsModel->notifyRowChanged(currentOutput);

    emit currentProfileChanged();
    emit profileAdded(output->OutputId, profile.Id);
    setEditDirty(true);
}

void AudioMixerController::removeProfile(int index)
{
    const auto *constOutput = currentOutputData();
    if (!constOutput || index < 0 || index >= constOutput->Profiles.size() || constOutput->Profiles.size() <= 1)
        return;

    auto *output = mutableCurrentOutput();
    const QString profileId = output->Profiles.at(index).Id;
    const bool wasActive = profileId == output->ActiveProfileId;

    profilesModel->beginRemoveRows({}, index, index);
    output->Profiles.removeAt(index);
    profilesModel->endRemoveRows();

    if (wasActive)
        output->ActiveProfileId = output->Profiles.first().Id;

    if (isEditMode) {
        if (currentProfile > index || currentProfile >= output->Profiles.size())
            --currentProfile;
    } else {
        currentProfile = output->activeProfileIndex();
    }

    rulesModel->beginResetModel();
    rulesModel->endResetModel();
    profilesModel->notifyAllChanged();
    outputsModel->notifyRowChanged(currentOutput);

    emit currentProfileChanged();
    emit activeProfileChanged();
    emit profileRemoved(output->OutputId, profileId);

    if (isEditMode) {
        setEditDirty(true);
    } else {
        if (wasActive)
            applyActiveProfile(output->OutputId);
        emit hotkeysChanged();
        scheduleSave();
    }
}

void AudioMixerController::renameCurrentProfile(const QString &name)
{
    const QString trimmed = name.trimmed();
    const auto *constProfile = currentProfileData();
    if (!constProfile || trimmed.isEmpty() || constProfile->Name == trimmed)
        return;

    mutableCurrentProfile()->Name = trimmed;
    profilesModel->notifyRowChanged(currentProfile);
    outputsModel->notifyRowChanged(currentOutput);
    emit currentProfileChanged();
    profileEdited();
}

void AudioMixerController::setCurrentProfileHotkey(const QStringList &combo)
{
    const auto *constProfile = currentProfileData();
    if (!constProfile || constProfile->Hotkey == combo)
        return;

    mutableCurrentProfile()->Hotkey = combo;
    profilesModel->notifyRowChanged(currentProfile);
    emit currentProfileChanged();
    profileEdited(true);
}

// ---- Rules ----

void AudioMixerController::addRule(const QString &processName, const QString &guid, const QString &scope)
{
    const QString name = processName.trimmed();
    if (name.isEmpty() || !currentProfileData())
        return;

    ProcessRuleData rule;
    rule.ProcessName = name;
    rule.Guid = guid.trimmed();
    if (!scope.isEmpty())
        rule.Scope = scope;

    auto *profile = mutableCurrentProfile();
    const int row = static_cast<int>(profile->Rules.size());
    rulesModel->beginInsertRows({}, row, row);
    profile->Rules.append(rule);
    rulesModel->endInsertRows();

    profilesModel->notifyRowChanged(currentProfile);
    emit processRuleAdded(currentOutputData()->OutputId, profile->Id, name);
    if (!isEditMode && isCurrentProfileActive())
        emit processRuleChanged(currentOutputData()->OutputId, name, rule.Volume, rule.Muted);
    profileEdited();
    if (!isEditMode && isCurrentProfileActive())
        pushRules(currentOutputData()->OutputId);
    updateSessionFlags(currentOutput);
}

void AudioMixerController::removeRule(int index)
{
    const auto *constProfile = currentProfileData();
    if (!constProfile || index < 0 || index >= constProfile->Rules.size())
        return;

    auto *profile = mutableCurrentProfile();
    const QString processName = profile->Rules.at(index).ProcessName;

    rulesModel->beginRemoveRows({}, index, index);
    profile->Rules.removeAt(index);
    rulesModel->endRemoveRows();

    profilesModel->notifyRowChanged(currentProfile);
    emit processRuleRemoved(currentOutputData()->OutputId, profile->Id, processName);
    profileEdited();
    if (!isEditMode && isCurrentProfileActive())
        pushRules(currentOutputData()->OutputId);
}

void AudioMixerController::setRuleVolume(int index, double volume)
{
    const auto *constProfile = currentProfileData();
    if (!constProfile || index < 0 || index >= constProfile->Rules.size())
        return;

    volume = qBound(0.0, volume, 1.0);
    if (qFuzzyCompare(1.0 + constProfile->Rules.at(index).Volume, 1.0 + volume))
        return;

    auto &rule = mutableCurrentProfile()->Rules[index];
    rule.Volume = volume;
    rulesModel->notifyRowChanged(index, { ProcessRulesModel::VolumeRole });

    if (!isEditMode && isCurrentProfileActive())
        emit processRuleChanged(currentOutputData()->OutputId, rule.ProcessName, rule.Volume, rule.Muted);
    profileEdited();
    if (!isEditMode && isCurrentProfileActive())
        pushRules(currentOutputData()->OutputId);
}

void AudioMixerController::setRuleMuted(int index, bool muted)
{
    const auto *constProfile = currentProfileData();
    if (!constProfile || index < 0 || index >= constProfile->Rules.size()
        || constProfile->Rules.at(index).Muted == muted)
        return;

    auto &rule = mutableCurrentProfile()->Rules[index];
    rule.Muted = muted;
    rulesModel->notifyRowChanged(index, { ProcessRulesModel::MutedRole });

    if (!isEditMode && isCurrentProfileActive())
        emit processRuleChanged(currentOutputData()->OutputId, rule.ProcessName, rule.Volume, rule.Muted);
    profileEdited();
    if (!isEditMode && isCurrentProfileActive())
        pushRules(currentOutputData()->OutputId);
}

// ---- Sessions ----

void AudioMixerController::onSessionsChanged(const QString &deviceId, const QStringList &)
{
    updateSessionFlags(indexOfOutput(deviceId));
}

void AudioMixerController::onSessionsAppeared(const QString &deviceId)
{
    // A process started playing: its rule from the committed profile applies now
    pushRules(deviceId);
}

void AudioMixerController::updateSessionFlags(int outputIndex)
{
    if (outputIndex < 0 || outputIndex >= outputsList.size())
        return;

    const QStringList names = sessions->sessionProcesses(outputsList.at(outputIndex).OutputId);
    bool currentChanged = false;

    const auto &output = outputsList.at(outputIndex);
    for (int p = 0; p < output.Profiles.size(); ++p) {
        for (int r = 0; r < output.Profiles.at(p).Rules.size(); ++r) {
            const auto &rule = output.Profiles.at(p).Rules.at(r);
            const bool active = std::any_of(names.cbegin(), names.cend(),
                                            [&](const QString &exe) { return rule.matches(exe); });
            if (rule.SessionActive == active)
                continue;

            // Only write on change, so the edit snapshot isn't detached for nothing
            outputsList[outputIndex].Profiles[p].Rules[r].SessionActive = active;
            if (outputIndex == currentOutput && p == currentProfile)
                currentChanged = true;
        }
    }

    if (currentChanged)
        rulesModel->notifyAllChanged({ ProcessRulesModel::SessionActiveRole });
}

QVariantList AudioMixerController::runningProcesses() const
{
    const auto *output = currentOutputData();
    const QStringList playing = output ? sessions->sessionProcesses(output->OutputId) : QStringList();

    QStringList names = AudioSessionManager::runningProcessNames();
    std::stable_sort(names.begin(), names.end(), [&](const QString &a, const QString &b) {
        return playing.contains(a, Qt::CaseInsensitive) > playing.contains(b, Qt::CaseInsensitive);
    });

    QVariantList result;
    result.reserve(names.size());
    for (const auto &name : std::as_const(names)) {
        result.append(QVariantMap {
            { "name",     name },
            { "hasAudio", playing.contains(name, Qt::CaseInsensitive) }
        });
    }
    return result;
}

// ---- Hotkeys ----

void AudioMixerController::registerHotkeys()
{
    QList<GlobalHotkeyManager::Binding> bindings;
    for (const auto &output : committedOutputs()) {
        if (output.Removed)
            continue;
        for (const auto &profile : output.Profiles) {
            if (!profile.Hotkey.isEmpty())
                bindings.append({ output.OutputId + QLatin1Char('|') + profile.Id, profile.Hotkey });
        }
    }

    const QStringList failed = hotkeys->setHotkeys(bindings);
    failedHotkeys = QSet<QString>(failed.cbegin(), failed.cend());
    emit currentProfileChanged();
}

QString AudioMixerController::hotkeyKeyName(quint32 scanCode) const
{
    return GlobalHotkeyManager::keyNameFromScanCode(scanCode);
}

void AudioMixerController::onHotkeyActivated(const QString &key)
{
    if (isEditMode) {
        qDebug() << "Hotkey ignored while editing profiles";
        return;
    }

    activateProfileById(key.section(QLatin1Char('|'), 0, 0), key.section(QLatin1Char('|'), 1));
}

void AudioMixerController::activateProfileById(const QString &outputId, const QString &profileId)
{
    const int outputIndex = indexOfOutput(outputId);
    if (outputIndex < 0)
        return;

    const auto &profiles = outputsList.at(outputIndex).Profiles;
    for (int i = 0; i < profiles.size(); ++i) {
        if (profiles.at(i).Id == profileId) {
            activateProfile(outputIndex, i);
            return;
        }
    }
}

// ---- Edit mode ----

void AudioMixerController::beginEdit()
{
    if (isEditMode)
        return;

    saveTimer.stop();
    saveConfig();

    editSnapshot = outputsList; // implicitly shared, detaches on first write
    preEditOutputId = currentOutputData() ? currentOutputData()->OutputId : QString();
    isEditMode = true;
    setEditDirty(false);
    emit editModeChanged();
}

void AudioMixerController::acceptEdit()
{
    if (!isEditMode)
        return;

    editSnapshot.clear();
    finishEdit();
    saveConfig();

    for (const auto &output : std::as_const(outputsList)) {
        if (output.Online && !output.Removed)
            applyActiveProfile(output.OutputId);
    }
    emit hotkeysChanged();
}

void AudioMixerController::cancelEdit()
{
    if (!isEditMode)
        return;

    outputsList = editSnapshot;
    editSnapshot.clear();
    refreshOutputs();

    finishEdit();

    // Devices may have come and gone while editing
    syncEndpoints();
    for (int i = 0; i < outputsList.size(); ++i)
        updateSessionFlags(i);
}

void AudioMixerController::finishEdit()
{
    isEditMode = false;
    setEditDirty(false);

    // Back to the page of the profile that was active before editing
    int index = indexOfOutput(preEditOutputId);
    if (index < 0 && !outputsList.isEmpty())
        index = qBound(0, currentOutput, static_cast<int>(outputsList.size()) - 1);
    preEditOutputId.clear();

    setCurrentOutputInternal(index);
    emit editModeChanged();
}

void AudioMixerController::setEditDirty(bool dirty)
{
    if (isEditDirty == dirty)
        return;
    isEditDirty = dirty;
    emit editDirtyChanged();
}

void AudioMixerController::profileEdited(bool hotkeyChanged)
{
    if (isEditMode) {
        setEditDirty(true);
        return;
    }
    if (hotkeyChanged)
        emit hotkeysChanged();
    scheduleSave();
}

// ---- Apply ----

void AudioMixerController::applyActiveProfile(const QString &outputId)
{
    const auto &outputs = committedOutputs();
    const auto it = std::find_if(outputs.cbegin(), outputs.cend(),
                                 [&](const AudioOutputData &o) { return o.OutputId == outputId; });
    if (it == outputs.cend() || it->Removed || it->activeProfileIndex() < 0)
        return;

    const auto &profile = it->Profiles.at(it->activeProfileIndex());
    qDebug() << "Apply profile" << profile.Name << "on" << it->OutputName;
    emit profileActivated(outputId, profile.Id);
    sessions->applyProfile(outputId, profile);
}

void AudioMixerController::pushRules(const QString &outputId)
{
    const auto &outputs = committedOutputs();
    const auto it = std::find_if(outputs.cbegin(), outputs.cend(),
                                 [&](const AudioOutputData &o) { return o.OutputId == outputId; });
    if (it == outputs.cend() || it->Removed || it->activeProfileIndex() < 0)
        return;

    sessions->applyProfile(outputId, it->Profiles.at(it->activeProfileIndex()));
}

// ---- Persistence ----

void AudioMixerController::loadConfig()
{
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "No config yet at" << configPath();
        return;
    }

    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Config is corrupted, starting fresh:" << error.errorString();
        return;
    }

    outputsList.clear();
    for (const auto &value : doc.object().value("outputs").toArray()) {
        auto output = AudioOutputData::fromJson(value.toObject());
        if (output.OutputId.isEmpty() || indexOfOutput(output.OutputId) >= 0)
            continue;

        if (output.Profiles.isEmpty())
            output.Profiles.append(makeDefaultProfile());
        if (output.activeProfileIndex() >= 0)
            output.ActiveProfileId = output.Profiles.at(output.activeProfileIndex()).Id;

        outputsList.append(output);
    }
    refreshOutputs();

    qDebug() << "Loaded" << outputsList.size() << "known outputs from" << configPath();
}

void AudioMixerController::saveConfig()
{
    // In edit mode only the committed state goes to disk
    const auto &committed = isEditMode ? editSnapshot : outputsList;

    QJsonArray outputs;
    for (const auto &output : committed)
        outputs.append(output.toJson());

    const QJsonObject root {
        { "version", ConfigVersion },
        { "outputs", outputs }
    };

    QDir().mkpath(QFileInfo(configPath()).absolutePath());
    QSaveFile file(configPath());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Can't write config to" << configPath() << file.errorString();
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit())
        qWarning() << "Can't commit config" << file.errorString();
}

void AudioMixerController::scheduleSave()
{
    if (!isEditMode)
        saveTimer.start();
}

// ---- Helpers ----

void AudioMixerController::resetProfileModels()
{
    profilesModel->beginResetModel();
    profilesModel->endResetModel();
    rulesModel->beginResetModel();
    rulesModel->endResetModel();
}

int AudioMixerController::indexOfOutput(const QString &outputId) const
{
    if (outputId.isEmpty())
        return -1;
    for (int i = 0; i < outputsList.size(); ++i) {
        if (outputsList.at(i).OutputId == outputId)
            return i;
    }
    return -1;
}

bool AudioMixerController::isCurrentProfileActive() const
{
    return currentProfile >= 0 && currentProfile == activeProfileIndex();
}

QString AudioMixerController::uniqueProfileName(const AudioOutputData &output)
{
    for (int n = static_cast<int>(output.Profiles.size()) + 1;; ++n) {
        const QString candidate = QStringLiteral("Profile %1").arg(n);
        const bool taken = std::any_of(output.Profiles.cbegin(), output.Profiles.cend(),
                                       [&](const VolumeProfileData &p) { return p.Name == candidate; });
        if (!taken)
            return candidate;
    }
}

VolumeProfileData AudioMixerController::makeDefaultProfile()
{
    VolumeProfileData profile;
    profile.Name = QStringLiteral("Default");
    return profile;
}
