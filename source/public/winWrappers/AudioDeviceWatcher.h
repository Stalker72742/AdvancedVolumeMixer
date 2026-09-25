#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMetaObject>

#include <mmdeviceapi.h>
#include <atomic>

// PKEY_Device_FriendlyName и т.п. НЕ инклюдим тут: functiondiscoverykeys_devpkey.h
// подключается только в AudioDeviceWatcher.cpp вместе с INITGUID (см. комментарий
// там). Если инклюднуть его тут без INITGUID, сработает собственный
// include-guard системного заголовка, и повторный инклуд с INITGUID
// в .cpp будет пропущен — реальные данные PKEY_* так и не сгенерятся,
// и линковщик опять выдаст undefined reference.

struct AudioOutputInfo
{
    QString id;           // endpoint ID (стабильный, можно хранить в конфиге)
    QString name;         // friendly name ("Speakers (Realtek...)")
    bool    isDefault = false;
};

// Реализует IMMNotificationClient, наружу отдаёт только Qt-сигналы.
// COM-часть скрыта, юзать как обычный QObject.
class AudioDeviceWatcher : public QObject, private IMMNotificationClient
{
    Q_OBJECT
public:
    explicit AudioDeviceWatcher(QObject* parent = nullptr);
    ~AudioDeviceWatcher() override;

    AudioDeviceWatcher(const AudioDeviceWatcher&) = delete;
    AudioDeviceWatcher& operator=(const AudioDeviceWatcher&) = delete;

    // Синхронный снапшот текущих output-устройств (eRender, ACTIVE)
    QList<AudioOutputInfo> enumerateOutputs() const;

signals:
    // Дергается при добавлении/удалении/смене состояния устройства.
    // Один сигнал на любое изменение состава — этого обычно достаточно,
    // дальше сам решаешь, вызывать ли enumerateOutputs() заново.
    void endpointsChanged();

    // Отдельно — смена дефолтного устройства вывода (полезно для UI)
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