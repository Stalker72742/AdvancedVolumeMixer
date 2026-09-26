//
// Created by Stalker7274 on 25.09.2026.
//

#pragma once

#include <QObject>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QTimer>
#include <qqmlintegration.h>

#include "structs/AudioOutputData.h"
#include "AudioOutputsModel.h"
#include "MixerProfileModel.h"
#include "ProcessRulesModel.h"

class AudioDeviceWatcher;
class AudioSessionManager;
class GlobalHotkeyManager;

// Single source of truth for outputs -> profiles -> process rules.
//
// Normal mode: selecting a profile activates it, every edit is applied
// and saved immediately.
// Edit mode: the whole state is snapshotted, edits only touch the working
// copy; acceptEdit() commits + saves, cancelEdit() restores the snapshot.
// Both return the view to the output/profile that was active before.
class AudioMixerController : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Mixer)
    QML_SINGLETON

    // List models for QML
    Q_PROPERTY(AudioOutputsModel* outputs READ outputs CONSTANT)
    Q_PROPERTY(MixerProfileModel* profiles READ profiles CONSTANT)
    Q_PROPERTY(ProcessRulesModel* rules READ rules CONSTANT)

    // Output shown in the UI
    Q_PROPERTY(int currentOutputIndex READ currentOutputIndex NOTIFY currentOutputChanged)
    Q_PROPERTY(QString currentOutputName READ currentOutputName NOTIFY currentOutputChanged)
    Q_PROPERTY(bool currentOutputOnline READ currentOutputOnline NOTIFY currentOutputChanged)

    // Profile shown in the UI. Equals activeProfileIndex outside edit mode
    Q_PROPERTY(int currentProfileIndex READ currentProfileIndex NOTIFY currentProfileChanged)
    Q_PROPERTY(QString currentProfileName READ currentProfileName NOTIFY currentProfileChanged)
    Q_PROPERTY(QStringList currentProfileHotkey READ currentProfileHotkey NOTIFY currentProfileChanged)
    // Non-empty when the hotkey of the shown profile couldn't be registered
    Q_PROPERTY(QString currentProfileHotkeyError READ currentProfileHotkeyError NOTIFY currentProfileChanged)
    // Profile applied to the current output
    Q_PROPERTY(int activeProfileIndex READ activeProfileIndex NOTIFY activeProfileChanged)

    // Output list shows the removed outputs instead of the regular ones
    Q_PROPERTY(bool showRemoved READ showRemoved WRITE setShowRemoved NOTIFY showRemovedChanged)
    Q_PROPERTY(int removedCount READ removedCount NOTIFY removedCountChanged)

    Q_PROPERTY(bool editMode READ editMode NOTIFY editModeChanged)
    // Something was changed since beginEdit()
    Q_PROPERTY(bool editDirty READ editDirty NOTIFY editDirtyChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)

public:

    explicit AudioMixerController(QObject *parent = nullptr);
    ~AudioMixerController() override;

    [[nodiscard]] AudioOutputsModel* outputs() const { return outputsModel; }
    [[nodiscard]] MixerProfileModel* profiles() const { return profilesModel; }
    [[nodiscard]] ProcessRulesModel* rules() const { return rulesModel; }

    [[nodiscard]] int currentOutputIndex() const { return currentOutput; }
    [[nodiscard]] QString currentOutputName() const;
    [[nodiscard]] bool currentOutputOnline() const;

    [[nodiscard]] int currentProfileIndex() const { return currentProfile; }
    [[nodiscard]] QString currentProfileName() const;
    [[nodiscard]] QStringList currentProfileHotkey() const;
    [[nodiscard]] QString currentProfileHotkeyError() const;
    [[nodiscard]] int activeProfileIndex() const;

    [[nodiscard]] bool showRemoved() const { return isShowingRemoved; }
    void setShowRemoved(bool show);
    [[nodiscard]] int removedCount() const;

    [[nodiscard]] bool editMode() const { return isEditMode; }
    [[nodiscard]] bool editDirty() const { return isEditDirty; }
    [[nodiscard]] QString configPath() const;

    // Read access for the list models
    [[nodiscard]] const QList<AudioOutputData>& outputsData() const { return outputsList; }
    [[nodiscard]] const AudioOutputData* currentOutputData() const;
    [[nodiscard]] const VolumeProfileData* currentProfileData() const;

    // Output indexes are the model's "idx" role (index in outputsData())
    Q_INVOKABLE void selectOutput(int index);
    // Hide an output: its profiles and hotkeys stop working, apps on it get
    // their full volume back. Not available in edit mode.
    Q_INVOKABLE void removeOutput(int index);
    // Bring a removed output back and apply its active profile
    Q_INVOKABLE void restoreOutput(int index);
    // Delete a removed, disconnected output with all its profiles for good
    Q_INVOKABLE void forgetOutput(int index);

    // Normal mode: activate and apply the profile. Edit mode: only show it
    Q_INVOKABLE void selectProfile(int index);
    // Add a profile to the current output; enters edit mode so Cancel undoes it
    Q_INVOKABLE void addProfile();
    // Delete a profile of the current output, the last one can't be deleted
    Q_INVOKABLE void removeProfile(int index);
    Q_INVOKABLE void renameCurrentProfile(const QString &name);
    // combo like {"Ctrl", "Shift", "M"}, empty list clears the hotkey
    Q_INVOKABLE void setCurrentProfileHotkey(const QStringList &combo);

    // Rules of the shown profile. scope: "single", "group" or "all"
    Q_INVOKABLE void addRule(const QString &processName, const QString &guid, const QString &scope);
    Q_INVOKABLE void removeRule(int index);
    // volume in 0..1
    Q_INVOKABLE void setRuleVolume(int index, double volume);
    Q_INVOKABLE void setRuleMuted(int index, bool muted);

    // [{ name, hasAudio }] for the process picker, processes playing on the
    // current output first. Rules are not limited to this list.
    Q_INVOKABLE QVariantList runningProcesses() const;

    // English name of a physical key for the hotkey recorder, see GlobalHotkeyManager
    Q_INVOKABLE QString hotkeyKeyName(quint32 scanCode) const;

    // Switch the active profile of any output (tray menu); no-op in edit mode
    Q_INVOKABLE void activateProfileById(const QString &outputId, const QString &profileId);

    // Snapshot the state; edits stay local until accepted
    Q_INVOKABLE void beginEdit();
    // Keep the edits: save, apply active profiles, re-register hotkeys
    Q_INVOKABLE void acceptEdit();
    // Throw the edits away and restore the snapshot
    Q_INVOKABLE void cancelEdit();

signals:

    void currentOutputChanged();
    void currentProfileChanged();
    void activeProfileChanged();
    void editModeChanged();
    void showRemovedChanged();
    void removedCountChanged();
    void editDirtyChanged();

    // ---- Backend hooks ----
    // Output seen for the very first time (already persisted)
    void outputAdded(const QString &outputId, const QString &name);
    void outputConnected(const QString &outputId);
    void outputDisconnected(const QString &outputId);
    // Deleted for good with all its profiles
    void outputForgotten(const QString &outputId);
    // Hidden by the user / brought back
    void outputRemoved(const QString &outputId);
    void outputRestored(const QString &outputId);

    void profileAdded(const QString &outputId, const QString &profileId);
    void profileRemoved(const QString &outputId, const QString &profileId);
    // Committed profile must be applied to the output (switch, accept, reconnect)
    void profileActivated(const QString &outputId, const QString &profileId);

    void processRuleAdded(const QString &outputId, const QString &profileId, const QString &processName);
    void processRuleRemoved(const QString &outputId, const QString &profileId, const QString &processName);
    // Live change of a rule in the active profile (normal mode only)
    void processRuleChanged(const QString &outputId, const QString &processName, double volume, bool muted);

    // Committed hotkeys changed, re-register global hotkeys
    void hotkeysChanged();

protected:

    // profiles.json in the app config folder
    void loadConfig();
    // Writes the committed state (the snapshot while editing)
    void saveConfig();
    // Debounced saveConfig(), skipped in edit mode
    void scheduleSave();

    // Merge the live device list into the known outputs: add new ones,
    // update names/online/default flags, apply profiles on reconnect
    void syncEndpoints();

    // Committed state: in edit mode that's the snapshot, not the working copy
    [[nodiscard]] const QList<AudioOutputData>& committedOutputs() const { return isEditMode ? editSnapshot : outputsList; }

    // Logs + profileActivated + pushes the committed active profile to the device
    void applyActiveProfile(const QString &outputId);
    // Silent push of the committed active profile (live edits, new sessions)
    void pushRules(const QString &outputId);
    // Normal mode switch of the active profile of any output (click, hotkey)
    void activateProfile(int outputIndex, int profileIndex);

    void onSessionsChanged(const QString &deviceId, const QStringList &processNames);
    // New audio session on a device: re-apply its rules so the new app gets them
    void onSessionsAppeared(const QString &deviceId);
    // Refresh the "has audio session" flag of every rule on the output
    void updateSessionFlags(int outputIndex);

    // Register hotkeys of all committed, non-removed profiles
    void registerHotkeys();
    // key is "outputId|profileId"
    void onHotkeyActivated(const QString &key);

    // Called after any edit of the current profile
    void profileEdited(bool hotkeyChanged = false);
    void setEditDirty(bool dirty);
    // Leave edit mode and go back to the output shown before beginEdit()
    void finishEdit();

    // Profiles and rules models point at a different output/profile now
    void resetProfileModels();
    // Output list changed structurally or a Removed flag flipped
    void refreshOutputs();
    // Online, not removed outputs get their sessions polled
    void updateWatchedSessions();
    // Best output to show: Windows default, then any online, then any; never a removed one
    [[nodiscard]] int firstVisibleOutput() const;
    // Show an output and its active profile
    void setCurrentOutputInternal(int index);

    [[nodiscard]] int indexOfOutput(const QString &outputId) const;
    [[nodiscard]] bool isCurrentProfileActive() const;
    [[nodiscard]] static QString uniqueProfileName(const AudioOutputData &output);
    [[nodiscard]] static VolumeProfileData makeDefaultProfile();

    // Mutable access; detaches from the edit snapshot on first write
    AudioOutputData* mutableCurrentOutput();
    VolumeProfileData* mutableCurrentProfile();

    AudioDeviceWatcher*  watcher = nullptr;
    AudioSessionManager* sessions = nullptr;
    GlobalHotkeyManager* hotkeys = nullptr;
    AudioOutputsModel*  outputsModel = nullptr;
    MixerProfileModel*  profilesModel = nullptr;
    ProcessRulesModel*  rulesModel = nullptr;

    QList<AudioOutputData> outputsList;  // working state
    QList<AudioOutputData> editSnapshot; // committed state while editing
    QString preEditOutputId;
    QSet<QString> failedHotkeys; // "outputId|profileId" that RegisterHotKey refused

    int  currentOutput = -1;
    int  currentProfile = -1;
    bool isEditMode = false;
    bool isShowingRemoved = false;
    bool isEditDirty = false;

    QTimer saveTimer;
};
