// Порядок инклудов тут принципиален для MinGW:
// functiondiscoverykeys_devpkey.h использует PROPERTYKEY/REFPROPERTYKEY
// и макрос EXTERN_C, которые объявляются в windows.h/propidl.h.
// Поэтому AudioDeviceWatcher.h (тянущий mmdeviceapi.h -> windows.h)
// должен быть подключен ПЕРВЫМ, а INITGUID-блок — уже после.
#include "AudioDeviceWatcher.h"

// INITGUID заставляет functiondiscoverykeys_devpkey.h не просто
// объявить PKEY_* как extern, а реально определить их данные.
// На MSVC эти символы приезжают из Propsys.lib, но в дистрибутивах
// MinGW такого import-lib с данными нет — без INITGUID будет
// "undefined reference to PKEY_Device_FriendlyName" на линковке.
// Важно: определять только в ОДНОМ .cpp файле проекта, иначе
// поймаешь multiple definition при инклуде из нескольких TU.
#define INITGUID
#include <functiondiscoverykeys_devpkey.h>
#undef INITGUID

#include <QDebug>
#include <combaseapi.h>
#include <propvarutil.h>

AudioDeviceWatcher::AudioDeviceWatcher(QObject* parent)
    : QObject(parent)
{
    // Если у тебя уже есть CoInitializeEx где-то выше по стеку (обычно да,
    // если это тот же поток, где живёт QApplication) — вызов ниже просто
    // вернёт S_FALSE, это ок. Если инициализируешь тут впервые — не забудь
    // CoUninitialize() в деструкторе (см. m_comInitializedHere).
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
    // Не удаляем this через delete — этот объект живёт как QObject
    // и им владеет Qt (родитель/стек/умный указатель). Просто держим
    // счётчик в адеквате для COM-контракта.
    ULONG count = --m_refCount;
    return count;
}

// ---- IMMNotificationClient ----
// ВАЖНО: эти методы вызываются COM-ом на служебном потоке, НЕ на твоём
// Qt-потоке. Поэтому сигнал не эмиттим напрямую — прогоняем через
// invokeMethod с QueuedConnection, чтобы слот выполнился в потоке,
// которому принадлежит this (обычно main/GUI thread).

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
    // Переименование устройства и т.п. — тоже считаем изменением состава
    emitEndpointsChangedQueued();
    return S_OK;
}