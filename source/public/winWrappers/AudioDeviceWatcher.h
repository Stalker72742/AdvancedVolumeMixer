#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMetaObject>

#include <mmdeviceapi.h>
#include <atomic>

// Don't include functiondiscoverykeys_devpkey.h here: the .cpp includes it
// with INITGUID to define the PKEY_* data. Including it earlier without
// INITGUID would trip its include guard and the PKEY_* symbols would stay
// undefined at link time.

struct AudioOutputInfo
{
    QString id;           // endpoint ID, stable across reboots (safe to store)
    QString name;         // friendly name, e.g. "Speakers (Realtek...)"
    bool    isDefault = false;
};

// Lists audio outputs and reports device changes as Qt signals.
// Implements IMMNotificationClient internally, use it as a plain QObject.
class AudioDeviceWatcher : public QObject, private IMMNotificationClient
{
    Q_OBJECT
public:
    explicit AudioDeviceWatcher(QObject* parent = nullptr);
    ~AudioDeviceWatcher() override;

    AudioDeviceWatcher(const AudioDeviceWatcher&) = delete;
    AudioDeviceWatcher& operator=(const AudioDeviceWatcher&) = delete;

    // Snapshot of the active output devices (eRender, DEVICE_STATE_ACTIVE)
    QList<AudioOutputInfo> enumerateOutputs() const;

signals:
    // Any device was added, removed, enabled/disabled or renamed.
    // Call enumerateOutputs() again to get the new state.
    void endpointsChanged();

    // Windows default output (multimedia role) changed
    void defaultOutputChanged(const QString& newDefaultId);

private:
    // ---- IUnknown ----
    HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG   __stdcall AddRef() override;
    ULONG   __stdcall Release() override;

    // ---- IMMNotificationClient ----
    HRESULT __stdcall OnDeviceStateChanged(LPCWSTR deviceId, DWORD newState) override;
    HRESULT __stdcall OnDeviceAdded(LPCWSTR deviceId) override;
    HRESULT __stdcall OnDeviceRemoved(LPCWSTR deviceId) override;
    HRESULT __stdcall OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR defaultDeviceId) override;
    HRESULT __stdcall OnPropertyValueChanged(LPCWSTR deviceId, const PROPERTYKEY key) override;

    void emitEndpointsChangedQueued();

    IMMDeviceEnumerator* m_enumerator = nullptr;
    std::atomic<ULONG>   m_refCount{1};
    bool                 m_comInitializedHere = false;
};