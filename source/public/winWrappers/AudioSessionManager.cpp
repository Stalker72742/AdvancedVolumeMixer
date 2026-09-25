#include "AudioSessionManager.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <tlhelp32.h>

#include <QDebug>
#include <QFileInfo>

namespace
{
    constexpr int PollIntervalMs = 1500;

    // Minimal COM smart pointer, releases on scope exit
    template<typename T>
    class ComPtr
    {
    public:
        ComPtr() = default;
        ~ComPtr() { if (ptr) ptr->Release(); }
        ComPtr(const ComPtr&) = delete;
        ComPtr& operator=(const ComPtr&) = delete;

        T** put() { return &ptr; }
        void** putVoid() { return reinterpret_cast<void**>(&ptr); }
        T* get() const { return ptr; }
        T* operator->() const { return ptr; }
        explicit operator bool() const { return ptr != nullptr; }

    private:
        T* ptr = nullptr;
    };

    QString processNameForPid(DWORD pid)
    {
        if (pid == 0)
            return {};

        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!process)
            return {};

        wchar_t buffer[MAX_PATH];
        DWORD size = MAX_PATH;
        QString name;
        if (QueryFullProcessImageNameW(process, 0, buffer, &size))
            name = QFileInfo(QString::fromWCharArray(buffer, static_cast<int>(size))).fileName();
        CloseHandle(process);
        return name;
    }
}

AudioSessionManager::AudioSessionManager(QObject* parent)
    : QObject(parent)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    comInitializedHere = SUCCEEDED(hr) && hr != S_FALSE;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr))
        qWarning() << "AudioSessionManager: CoCreateInstance failed, hr =" << hr;

    pollTimer.setInterval(PollIntervalMs);
    connect(&pollTimer, &QTimer::timeout, this, &AudioSessionManager::poll);
    pollTimer.start();
}

AudioSessionManager::~AudioSessionManager()
{
    if (enumerator) {
        enumerator->Release();
        enumerator = nullptr;
    }
    if (comInitializedHere)
        CoUninitialize();
}

void AudioSessionManager::setWatchedDevices(const QStringList& deviceIds)
{
    watched = deviceIds;

    // Forget state of devices that went away
    for (auto it = lastNames.begin(); it != lastNames.end();) {
        if (!watched.contains(it.key())) {
            emit sessionsChanged(it.key(), {});
            knownSessions.remove(it.key());
            touchedSessions.remove(it.key());
            it = lastNames.erase(it);
        } else {
            ++it;
        }
    }

    poll();
}

template<typename Fn>
void AudioSessionManager::forEachSession(const QString& deviceId, Fn&& fn) const
{
    if (!enumerator || deviceId.isEmpty())
        return;

    ComPtr<IMMDevice> device;
    if (FAILED(enumerator->GetDevice(reinterpret_cast<LPCWSTR>(deviceId.utf16()), device.put())))
        return;

    ComPtr<IAudioSessionManager2> manager;
    if (FAILED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, manager.putVoid())))
        return;

    ComPtr<IAudioSessionEnumerator> sessions;
    if (FAILED(manager->GetSessionEnumerator(sessions.put())))
        return;

    int count = 0;
    sessions->GetCount(&count);

    for (int i = 0; i < count; ++i) {
        ComPtr<IAudioSessionControl> control;
        if (FAILED(sessions->GetSession(i, control.put())))
            continue;

        AudioSessionState state = AudioSessionStateExpired;
        if (FAILED(control->GetState(&state)) || state == AudioSessionStateExpired)
            continue;

        ComPtr<IAudioSessionControl2> control2;
        if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2), control2.putVoid())))
            continue;
        if (control2->IsSystemSoundsSession() == S_OK)
            continue;

        DWORD pid = 0;
        control2->GetProcessId(&pid);
        const QString exeName = processNameForPid(pid);
        if (exeName.isEmpty())
            continue;

        QString instanceId;
        LPWSTR instance = nullptr;
        if (SUCCEEDED(control2->GetSessionInstanceIdentifier(&instance)) && instance) {
            instanceId = QString::fromWCharArray(instance);
            CoTaskMemFree(instance);
        } else {
            instanceId = QStringLiteral("%1|%2").arg(pid).arg(i);
        }

        fn(control.get(), exeName, instanceId);
    }
}

void AudioSessionManager::poll()
{
    for (const auto& deviceId : std::as_const(watched)) {
        QSet<QString> ids;
        QStringList names;

        forEachSession(deviceId, [&](IAudioSessionControl*, const QString& exeName, const QString& instanceId) {
            ids.insert(instanceId);
            if (!names.contains(exeName, Qt::CaseInsensitive))
                names.append(exeName);
        });
        names.sort(Qt::CaseInsensitive);

        auto& known = knownSessions[deviceId];
        const bool appeared = !(ids - known).isEmpty();
        known = ids;
        touchedSessions[deviceId].intersect(ids);

        auto& last = lastNames[deviceId];
        if (last != names) {
            last = names;
            emit sessionsChanged(deviceId, names);
        }
        if (appeared)
            emit sessionsAppeared(deviceId);
    }
}

void AudioSessionManager::applyProfile(const QString& deviceId, const VolumeProfileData& profile)
{
    auto& touched = touchedSessions[deviceId];

    forEachSession(deviceId, [&](IAudioSessionControl* control, const QString& exeName, const QString& instanceId) {
        const ProcessRuleData* rule = profile.ruleFor(exeName);
        if (!rule && !touched.contains(instanceId))
            return;

        ComPtr<ISimpleAudioVolume> volume;
        if (FAILED(control->QueryInterface(__uuidof(ISimpleAudioVolume), volume.putVoid())))
            return;

        if (rule) {
            volume->SetMasterVolume(static_cast<float>(rule->Volume), nullptr);
            volume->SetMute(rule->Muted ? TRUE : FALSE, nullptr);
            touched.insert(instanceId);
        } else {
            // Not ruled by the new profile anymore, give it back its full volume
            volume->SetMasterVolume(1.0f, nullptr);
            volume->SetMute(FALSE, nullptr);
            touched.remove(instanceId);
        }
    });
}

QStringList AudioSessionManager::runningProcessNames()
{
    QStringList names;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return names;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            const QString name = QString::fromWCharArray(entry.szExeFile);
            // Skip pseudo processes ("[System Process]", "System", "Registry", ...)
            if (entry.th32ProcessID <= 4 || !name.contains(QLatin1Char('.')))
                continue;
            if (!names.contains(name, Qt::CaseInsensitive))
                names.append(name);
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);

    names.sort(Qt::CaseInsensitive);
    return names;
}
