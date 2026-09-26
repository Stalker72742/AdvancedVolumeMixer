// Include order matters on MinGW: functiondiscoverykeys_devpkey.h needs
// PROPERTYKEY and EXTERN_C from windows.h, which comes with our header
// (via mmdeviceapi.h). So our header goes first, the INITGUID block after.
#include "AudioDeviceWatcher.h"

// INITGUID makes the header define the PKEY_* data instead of declaring it
// extern. MSVC gets them from Propsys.lib, MinGW has no such library and
// fails with "undefined reference to PKEY_Device_FriendlyName".
// Do this in one .cpp only, or the symbols get defined twice.
#define INITGUID
#include <functiondiscoverykeys_devpkey.h>
#undef INITGUID

#include <QDebug>
#include <combaseapi.h>
#include <propvarutil.h>

AudioDeviceWatcher::AudioDeviceWatcher(QObject* parent)
    : QObject(parent)
{
    // S_FALSE means COM is already initialized on this thread (usual for the
    // GUI thread). Only balance with CoUninitialize() if we initialized it.
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    m_comInitializedHere = SUCCEEDED(hr) && hr != S_FALSE;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                           __uuidof(IMMDeviceEnumerator),
                           reinterpret_cast<void**>(&m_enumerator));
    if (FAILED(hr)) {
        qWarning() << "AudioDeviceWatcher: CoCreateInstance failed, hr =" << hr;
        return;
    }

    hr = m_enumerator->RegisterEndpointNotificationCallback(this);
    if (FAILED(hr)) {
        qWarning() << "AudioDeviceWatcher: RegisterEndpointNotificationCallback failed, hr =" << hr;
    }
}

AudioDeviceWatcher::~AudioDeviceWatcher()
{
    if (m_enumerator) {
        m_enumerator->UnregisterEndpointNotificationCallback(this);
        m_enumerator->Release();
        m_enumerator = nullptr;
    }
    if (m_comInitializedHere) {
        CoUninitialize();
    }
}

QList<AudioOutputInfo> AudioDeviceWatcher::enumerateOutputs() const
{
    QList<AudioOutputInfo> result;
    if (!m_enumerator)
        return result;

    IMMDeviceCollection* collection = nullptr;
    HRESULT hr = m_enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
    if (FAILED(hr) || !collection)
        return result;

    IMMDevice* defaultDevice = nullptr;
    QString defaultId;
    if (SUCCEEDED(m_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &defaultDevice)) && defaultDevice) {
        LPWSTR idStr = nullptr;
        if (SUCCEEDED(defaultDevice->GetId(&idStr))) {
            defaultId = QString::fromWCharArray(idStr);
            CoTaskMemFree(idStr);
        }
        defaultDevice->Release();
    }

    UINT count = 0;
    collection->GetCount(&count);

    for (UINT i = 0; i < count; ++i) {
        IMMDevice* device = nullptr;
        if (FAILED(collection->Item(i, &device)) || !device)
            continue;

        AudioOutputInfo info;

        LPWSTR idStr = nullptr;
        if (SUCCEEDED(device->GetId(&idStr))) {
            info.id = QString::fromWCharArray(idStr);
            CoTaskMemFree(idStr);
        }

        IPropertyStore* props = nullptr;
        if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &props)) && props) {
            PROPVARIANT nameVar;
            PropVariantInit(&nameVar);
            if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &nameVar))) {
                info.name = QString::fromWCharArray(nameVar.pwszVal);
            }
            PropVariantClear(&nameVar);
            props->Release();
        }

        info.isDefault = (!defaultId.isEmpty() && info.id == defaultId);

        result.append(info);
        device->Release();
    }

    collection->Release();
    return result;
}

// ---- IUnknown ----

HRESULT AudioDeviceWatcher::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject) return E_POINTER;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient)) {
        *ppvObject = static_cast<IMMNotificationClient*>(this);
        AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG AudioDeviceWatcher::AddRef()
{
    return ++m_refCount;
}

ULONG AudioDeviceWatcher::Release()
{
    // Lifetime is owned by Qt (QObject parent), never delete this here.
    // The counter only keeps the COM contract consistent.
    ULONG count = --m_refCount;
    return count;
}

// ---- IMMNotificationClient ----
// COM calls these on its own worker thread. Signals are re-emitted through
// a queued invokeMethod so slots run on the thread that owns this object.

void AudioDeviceWatcher::emitEndpointsChangedQueued()
{
    QMetaObject::invokeMethod(this, [this]() {
        emit endpointsChanged();
    }, Qt::QueuedConnection);
}

HRESULT AudioDeviceWatcher::OnDeviceStateChanged(LPCWSTR, DWORD)
{
    emitEndpointsChangedQueued();
    return S_OK;
}

HRESULT AudioDeviceWatcher::OnDeviceAdded(LPCWSTR)
{
    emitEndpointsChangedQueued();
    return S_OK;
}

HRESULT AudioDeviceWatcher::OnDeviceRemoved(LPCWSTR)
{
    emitEndpointsChangedQueued();
    return S_OK;
}

HRESULT AudioDeviceWatcher::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR defaultDeviceId)
{
    if (flow == eRender && role == eMultimedia) {
        QString id = defaultDeviceId ? QString::fromWCharArray(defaultDeviceId) : QString();
        QMetaObject::invokeMethod(this, [this, id]() {
            emit defaultOutputChanged(id);
        }, Qt::QueuedConnection);
    }
    return S_OK;
}

HRESULT AudioDeviceWatcher::OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY)
{
    // Renames and other property changes are treated as a list change too
    emitEndpointsChangedQueued();
    return S_OK;
}