#include "GlobalHotkeyManager.h"

#include <windows.h>

#include <QCoreApplication>
#include <QDebug>

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

namespace
{
    struct KeyInfo
    {
        quint32     scanCode; // set 1, 0x100 = extended (E0 prefix), as in Qt's nativeScanCode
        const char* name;     // US QWERTY name shown in the UI and stored in the config
        UINT        vk;       // virtual key for RegisterHotKey
    };

    // Modifiers are listed for display only, RegisterHotKey takes them as flags.
    // VK codes of letters/digits/OEM keys stay on the same physical keys in the
    // Cyrillic layouts, so the registered hotkey fires in any layout.
    constexpr KeyInfo Keys[] = {
        { 0x001, "Esc", VK_ESCAPE },
        { 0x002, "1", '1' }, { 0x003, "2", '2' }, { 0x004, "3", '3' }, { 0x005, "4", '4' }, { 0x006, "5", '5' },
        { 0x007, "6", '6' }, { 0x008, "7", '7' }, { 0x009, "8", '8' }, { 0x00A, "9", '9' }, { 0x00B, "0", '0' },
        { 0x00C, "-", VK_OEM_MINUS }, { 0x00D, "=", VK_OEM_PLUS }, { 0x00E, "Backspace", VK_BACK }, { 0x00F, "Tab", VK_TAB },
        { 0x010, "Q", 'Q' }, { 0x011, "W", 'W' }, { 0x012, "E", 'E' }, { 0x013, "R", 'R' }, { 0x014, "T", 'T' },
        { 0x015, "Y", 'Y' }, { 0x016, "U", 'U' }, { 0x017, "I", 'I' }, { 0x018, "O", 'O' }, { 0x019, "P", 'P' },
        { 0x01A, "[", VK_OEM_4 }, { 0x01B, "]", VK_OEM_6 }, { 0x01C, "Enter", VK_RETURN }, { 0x01D, "Ctrl", VK_CONTROL },
        { 0x01E, "A", 'A' }, { 0x01F, "S", 'S' }, { 0x020, "D", 'D' }, { 0x021, "F", 'F' }, { 0x022, "G", 'G' },
        { 0x023, "H", 'H' }, { 0x024, "J", 'J' }, { 0x025, "K", 'K' }, { 0x026, "L", 'L' },
        { 0x027, ";", VK_OEM_1 }, { 0x028, "'", VK_OEM_7 }, { 0x029, "`", VK_OEM_3 }, { 0x02A, "Shift", VK_SHIFT },
        { 0x02B, "\\", VK_OEM_5 },
        { 0x02C, "Z", 'Z' }, { 0x02D, "X", 'X' }, { 0x02E, "C", 'C' }, { 0x02F, "V", 'V' }, { 0x030, "B", 'B' },
        { 0x031, "N", 'N' }, { 0x032, "M", 'M' },
        { 0x033, ",", VK_OEM_COMMA }, { 0x034, ".", VK_OEM_PERIOD }, { 0x035, "/", VK_OEM_2 }, { 0x036, "Shift", VK_SHIFT },
        { 0x037, "Num*", VK_MULTIPLY }, { 0x038, "Alt", VK_MENU }, { 0x039, "Space", VK_SPACE }, { 0x03A, "CapsLock", VK_CAPITAL },
        { 0x03B, "F1", VK_F1 }, { 0x03C, "F2", VK_F2 }, { 0x03D, "F3", VK_F3 }, { 0x03E, "F4", VK_F4 }, { 0x03F, "F5", VK_F5 },
        { 0x040, "F6", VK_F6 }, { 0x041, "F7", VK_F7 }, { 0x042, "F8", VK_F8 }, { 0x043, "F9", VK_F9 }, { 0x044, "F10", VK_F10 },
        { 0x046, "ScrollLock", VK_SCROLL },
        { 0x047, "Num7", VK_NUMPAD7 }, { 0x048, "Num8", VK_NUMPAD8 }, { 0x049, "Num9", VK_NUMPAD9 }, { 0x04A, "Num-", VK_SUBTRACT },
        { 0x04B, "Num4", VK_NUMPAD4 }, { 0x04C, "Num5", VK_NUMPAD5 }, { 0x04D, "Num6", VK_NUMPAD6 }, { 0x04E, "Num+", VK_ADD },
        { 0x04F, "Num1", VK_NUMPAD1 }, { 0x050, "Num2", VK_NUMPAD2 }, { 0x051, "Num3", VK_NUMPAD3 }, { 0x052, "Num0", VK_NUMPAD0 },
        { 0x053, "Num.", VK_DECIMAL }, { 0x057, "F11", VK_F11 }, { 0x058, "F12", VK_F12 },
        { 0x064, "F13", VK_F13 }, { 0x065, "F14", VK_F14 }, { 0x066, "F15", VK_F15 }, { 0x067, "F16", VK_F16 },
        { 0x068, "F17", VK_F17 }, { 0x069, "F18", VK_F18 }, { 0x06A, "F19", VK_F19 }, { 0x06B, "F20", VK_F20 },
        { 0x06C, "F21", VK_F21 }, { 0x06D, "F22", VK_F22 }, { 0x06E, "F23", VK_F23 }, { 0x076, "F24", VK_F24 },
        { 0x11C, "NumEnter", VK_RETURN }, { 0x11D, "Ctrl", VK_CONTROL }, { 0x135, "Num/", VK_DIVIDE },
        { 0x137, "PrintScreen", VK_SNAPSHOT }, { 0x138, "Alt", VK_MENU },
        { 0x147, "Home", VK_HOME }, { 0x148, "Up", VK_UP }, { 0x149, "PgUp", VK_PRIOR }, { 0x14B, "Left", VK_LEFT },
        { 0x14D, "Right", VK_RIGHT }, { 0x14F, "End", VK_END }, { 0x150, "Down", VK_DOWN }, { 0x151, "PgDown", VK_NEXT },
        { 0x152, "Ins", VK_INSERT }, { 0x153, "Del", VK_DELETE },
        { 0x15B, "Win", VK_LWIN }, { 0x15C, "Win", VK_RWIN }, { 0x15D, "Menu", VK_APPS },
    };

    UINT keyToVk(const QString& key)
    {
        for (const auto& info : Keys) {
            if (key == QLatin1String(info.name))
                return info.vk;
        }
        return 0;
    }

    // Exactly one non-modifier key is required by RegisterHotKey
    bool parseCombo(const QStringList& combo, UINT& modifiers, UINT& vk)
    {
        modifiers = MOD_NOREPEAT;
        vk = 0;
        for (const auto& key : combo) {
            if (key == "Ctrl")       modifiers |= MOD_CONTROL;
            else if (key == "Shift") modifiers |= MOD_SHIFT;
            else if (key == "Alt")   modifiers |= MOD_ALT;
            else if (key == "Win")   modifiers |= MOD_WIN;
            else {
                if (vk != 0)
                    return false;
                vk = keyToVk(key);
                if (vk == 0)
                    return false;
            }
        }
        return vk != 0;
    }
}

QString GlobalHotkeyManager::keyNameFromScanCode(quint32 scanCode)
{
    for (const auto& info : Keys) {
        if (info.scanCode == scanCode)
            return QString::fromLatin1(info.name);
    }
    return {};
}

GlobalHotkeyManager::GlobalHotkeyManager(QObject* parent)
    : QObject(parent)
{
    if (auto* app = QCoreApplication::instance())
        app->installNativeEventFilter(this);
}

GlobalHotkeyManager::~GlobalHotkeyManager()
{
    unregisterAll();
    if (auto* app = QCoreApplication::instance())
        app->removeNativeEventFilter(this);
}

QStringList GlobalHotkeyManager::setHotkeys(const QList<Binding>& bindings)
{
    unregisterAll();

    QStringList failed;
    for (const auto& binding : bindings) {
        UINT modifiers = 0;
        UINT vk = 0;
        if (!parseCombo(binding.combo, modifiers, vk)) {
            qWarning() << "Hotkey is not representable:" << binding.combo;
            failed.append(binding.key);
            continue;
        }

        const int id = nextId++;
        if (!RegisterHotKey(nullptr, id, modifiers, vk)) {
            qWarning() << "Hotkey is already taken:" << binding.combo;
            failed.append(binding.key);
            continue;
        }
        registered.insert(id, binding.key);
    }
    return failed;
}

void GlobalHotkeyManager::unregisterAll()
{
    for (auto it = registered.cbegin(); it != registered.cend(); ++it)
        UnregisterHotKey(nullptr, it.key());
    registered.clear();
}

bool GlobalHotkeyManager::nativeEventFilter(const QByteArray& eventType, void* message, qintptr*)
{
    if (eventType != "windows_generic_MSG")
        return false;

    const auto* msg = static_cast<MSG*>(message);
    if (msg->message != WM_HOTKEY || msg->hwnd != nullptr)
        return false;

    const auto it = registered.constFind(static_cast<int>(msg->wParam));
    if (it == registered.cend())
        return false;

    emit activated(it.value());
    return true;
}
