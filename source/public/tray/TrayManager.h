#pragma once

#include <QList>
#include <QObject>
#include <QPointer>
#include <QSystemTrayIcon>

class QMenu;
class QQuickWindow;
class AudioMixerController;

// Keeps the app alive in the notification area: closing the window only
// hides it, the backend (hotkeys, session polling) keeps running.
// Quit from the tray menu is the only way to exit.
//
// The menu is a Qt-drawn QMenu ("windows:menus=none" platform option in
// main.cpp): the native Win32 menu can't be themed dark, and rebuilding its
// submenus from aboutToShow crashes Qt when the native popup closes.
class TrayManager : public QObject
{
    Q_OBJECT
public:
    TrayManager(QQuickWindow* window, AudioMixerController* mixer, QObject* parent = nullptr);
    ~TrayManager() override;

    [[nodiscard]] static bool isAvailable() { return QSystemTrayIcon::isSystemTrayAvailable(); }

    // Tray + window icon drawn at runtime, no resource file needed
    [[nodiscard]] static QIcon appIcon();

public slots:
    void showWindow();
    void hideWindow();
    void toggleWindow();

protected:
    void rebuildMenu();
    void updateToolTip();
    void onActivated(QSystemTrayIcon::ActivationReason reason);

    QMenu* createMenu(QWidget* parent = nullptr) const;

    QPointer<QQuickWindow> window;
    AudioMixerController*  mixer = nullptr;
    QSystemTrayIcon*       tray = nullptr;
    QMenu*                 menu = nullptr;
    QList<QMenu*>          submenus; // recreated on every rebuild
};
