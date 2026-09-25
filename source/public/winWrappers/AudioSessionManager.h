#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>

#include "structs/VolumeProfileData.h"

struct IMMDeviceEnumerator;
struct IAudioSessionControl;

// Per-app volume on a given output via WASAPI session API
// (IAudioSessionManager2 -> IAudioSessionControl2 / ISimpleAudioVolume).
//
// New sessions are found by polling: sessions come and go with every
// process that starts/stops playing, and a cheap enumeration on the GUI
// thread is far simpler than juggling IAudioSessionNotification plus
// per-session IAudioSessionEvents COM callbacks.
class AudioSessionManager : public QObject
{
    Q_OBJECT
public:
    explicit AudioSessionManager(QObject* parent = nullptr);
    ~AudioSessionManager() override;

    AudioSessionManager(const AudioSessionManager&) = delete;
    AudioSessionManager& operator=(const AudioSessionManager&) = delete;

    // Online endpoints to keep an eye on; polls them immediately
    void setWatchedDevices(const QStringList& deviceIds);

    // Pushes the profile's rules to every session on the device. Sessions
    // we changed earlier that no longer match any rule go back to 100%.
    void applyProfile(const QString& deviceId, const VolumeProfileData& profile);

    // Executables that have a session on the device, as of the last poll
    [[nodiscard]] QStringList sessionProcesses(const QString& deviceId) const { return lastNames.value(deviceId); }

    // Unique executable names of all running processes (Toolhelp snapshot)
    [[nodiscard]] static QStringList runningProcessNames();

signals:
    void sessionsChanged(const QString& deviceId, const QStringList& processNames);
    // A session that wasn't there on the previous poll (process started playing)
    void sessionsAppeared(const QString& deviceId);

protected:
    void poll();

    // fn(IAudioSessionControl*, exeName, sessionInstanceId)
    template<typename Fn>
    void forEachSession(const QString& deviceId, Fn&& fn) const;

    IMMDeviceEnumerator* enumerator = nullptr;
    bool comInitializedHere = false;

    QTimer pollTimer;
    QStringList watched;
    QHash<QString, QSet<QString>> knownSessions;  // deviceId -> session instance ids
    QHash<QString, QStringList>   lastNames;      // deviceId -> exe names
    QHash<QString, QSet<QString>> touchedSessions; // deviceId -> sessions we changed
};
