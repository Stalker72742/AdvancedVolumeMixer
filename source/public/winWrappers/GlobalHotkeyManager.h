#pragma once

#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QObject>
#include <QStringList>

// System-wide hotkeys via RegisterHotKey. Registered without a window,
// so WM_HOTKEY lands in the GUI thread queue and is caught by the
// native event filter.
class GlobalHotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit GlobalHotkeyManager(QObject* parent = nullptr);
    ~GlobalHotkeyManager() override;

    struct Binding
    {
        QString     key;   // caller-defined id, echoed back by activated()
        QStringList combo; // e.g. {"Ctrl", "Shift", "F13"}
    };

    // Replaces all registered hotkeys. Returns keys that failed to register
    // (combo already taken by another app/binding, or not representable).
    QStringList setHotkeys(const QList<Binding>& bindings);

    // US-layout name of a physical key ("M", "F13", "[", "Num5"...) from the
    // scan code Qt reports; empty when unknown. Layout independent, so a key
    // recorded on the Russian layout is the same hotkey as on the English one.
    [[nodiscard]] static QString keyNameFromScanCode(quint32 scanCode);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void activated(const QString& key);

protected:
    void unregisterAll();

    QHash<int, QString> registered; // hotkey id -> binding key
    int nextId = 1;
};
