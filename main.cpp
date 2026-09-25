#include <QApplication>
#include <QFont>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>

#include <memory>

#include "AudioMixerController.h"
#include "TrayManager.h"

int main(int argc, char* argv[]){
    // Tray menu must be a Qt-drawn QMenu (dark theme), see TrayManager.h.
    // Qt 6.8 ignores AA_DontUseNativeMenuWindows once a Quick Controls
    // ApplicationWindow exists and silently switches to native Win32 menus,
    // so native menus are turned off at the platform plugin level.
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "windows:menus=none");

    // QApplication (not QGuiApplication): QSystemTrayIcon and its menu live in Qt Widgets
    QApplication app(argc, argv);

    // Used by QStandardPaths for the config location (%LOCALAPPDATA%/Stalker7274/AdvancedVolumeMixer)
    QApplication::setOrganizationName("Stalker7274");
    QApplication::setApplicationName("AdvancedVolumeMixer");
    QApplication::setFont(QFont("Segoe UI", 10));
    QApplication::setWindowIcon(TrayManager::appIcon());

    const bool trayAvailable = TrayManager::isAvailable();
    // Hidden window != exit: only "Quit" in the tray menu ends the process
    QApplication::setQuitOnLastWindowClosed(!trayAvailable);

    QQmlApplicationEngine engine;
    QQuickStyle::setStyle("Material");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("AdvancedVolumeMixerModule", "Main");

    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0));
    if (!window)
        return -1;

    std::unique_ptr<TrayManager> tray;
    if (trayAvailable) {
        auto* mixer = engine.singletonInstance<AudioMixerController*>("AdvancedVolumeMixer.ComponentsModule", "Mixer");
        tray = std::make_unique<TrayManager>(window, mixer);
        window->setProperty("trayModeEnabled", true);
        // Main.qml asks instead of hiding itself, the tray owns window visibility
        QObject::connect(window, SIGNAL(hideToTrayRequested()), tray.get(), SLOT(hideWindow()));
    }

    return QApplication::exec();
}
