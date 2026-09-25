#include "TrayManager.h"
#include "AudioMixerController.h"

#include <QActionGroup>
#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QQuickWindow>
#include <QStyleFactory>

namespace
{
    // Same tokens as ui/theme/Theme.qml
    const QColor BgSecondary("#222327");
    const QColor BgHover("#323339");
    const QColor Accent("#5b8def");
    const QColor TextPrimary("#e6e6e8");
    const QColor TextSecondary("#9a9ba3");
    const QColor TextDisabled("#5c5d64");
    const QColor Border("#34353b");

    const char* MenuStyleSheet = R"(
        QMenu {
            background: #222327;
            color: #e6e6e8;
            border: 1px solid #34353b;
            padding: 6px 0;
        }
        QMenu::item {
            padding: 7px 28px 7px 30px;
            margin: 0 6px;
            border-radius: 4px;
        }
        QMenu::item:selected { background: #323339; }
        QMenu::item:disabled { color: #5c5d64; }
        QMenu::separator {
            height: 1px;
            background: #34353b;
            margin: 6px 12px;
        }
    )";

    QPalette darkPalette()
    {
        QPalette palette;
        palette.setColor(QPalette::Window, BgSecondary);
        palette.setColor(QPalette::Base, BgSecondary);
        palette.setColor(QPalette::WindowText, TextPrimary);
        palette.setColor(QPalette::Text, TextPrimary);
        palette.setColor(QPalette::ButtonText, TextPrimary);
        palette.setColor(QPalette::Button, BgSecondary);
        palette.setColor(QPalette::Highlight, BgHover);
        palette.setColor(QPalette::HighlightedText, TextPrimary);
        palette.setColor(QPalette::Mid, Border);
        palette.setColor(QPalette::Disabled, QPalette::Text, TextDisabled);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, TextDisabled);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, TextDisabled);
        return palette;
    }
}

TrayManager::TrayManager(QQuickWindow* window, AudioMixerController* mixer, QObject* parent)
    : QObject(parent)
    , window(window)
    , mixer(mixer)
    , tray(new QSystemTrayIcon(appIcon(), this))
    , menu(createMenu())
{
    // Refilled right before showing, so it always reflects the current profiles
    connect(menu, &QMenu::aboutToShow, this, &TrayManager::rebuildMenu);
    rebuildMenu();

    tray->setContextMenu(menu);
    connect(tray, &QSystemTrayIcon::activated, this, &TrayManager::onActivated);

    connect(mixer, &AudioMixerController::profileActivated, this, &TrayManager::updateToolTip);
    connect(mixer, &AudioMixerController::outputConnected, this, &TrayManager::updateToolTip);
    connect(mixer, &AudioMixerController::outputDisconnected, this, &TrayManager::updateToolTip);
    updateToolTip();

    tray->show();
}

TrayManager::~TrayManager()
{
    tray->hide();
    delete menu; // QMenu is a widget, it can't have a QObject parent
}

QMenu* TrayManager::createMenu(QWidget* parent) const
{
    auto* result = new QMenu(parent);
    // Fusion draws check/radio marks and arrows with the palette, native style ignores it
    static QStyle* fusion = QStyleFactory::create(QStringLiteral("Fusion"));
    result->setStyle(fusion);
    result->setPalette(darkPalette());
    result->setStyleSheet(QString::fromLatin1(MenuStyleSheet));
    result->setFont(QFont(QStringLiteral("Segoe UI"), 10));
    return result;
}

QIcon TrayManager::appIcon()
{
    QIcon icon;
    for (const int size : { 16, 20, 24, 32, 48, 64, 256 }) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Accent);
        painter.drawRoundedRect(QRectF(0, 0, size, size), size * 0.22, size * 0.22);

        // Three mixer faders
        painter.setBrush(Qt::white);
        const qreal barWidth = size * 0.12;
        const qreal heights[] = { 0.55, 0.30, 0.42 };
        for (int i = 0; i < 3; ++i) {
            const qreal x = size * (0.22 + i * 0.22);
            const qreal top = size * (0.80 - heights[i]);
            painter.drawRoundedRect(QRectF(x, top, barWidth, size * heights[i]), barWidth / 2, barWidth / 2);
        }
        painter.end();

        icon.addPixmap(pixmap);
    }
    return icon;
}

void TrayManager::showWindow()
{
    if (!window)
        return;
    if (window->visibility() == QWindow::Minimized || !window->isVisible())
        window->showNormal();
    window->raise();
    window->requestActivate();
}

void TrayManager::hideWindow()
{
    if (window)
        window->hide();
}

void TrayManager::toggleWindow()
{
    if (window && window->isVisible() && window->visibility() != QWindow::Minimized)
        hideWindow();
    else
        showWindow();
}

void TrayManager::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        toggleWindow();
}

void TrayManager::rebuildMenu()
{
    menu->clear();
    // clear() drops the actions only, submenus made by the previous rebuild are ours.
    // Deferred: this runs from aboutToShow, while Qt is still inside popup()
    for (auto* submenu : std::as_const(submenus))
        submenu->deleteLater();
    submenus.clear();

    auto* open = menu->addAction(QStringLiteral("Open Audio Profile Switcher"), this, &TrayManager::showWindow);
    QFont bold = open->font();
    bold.setBold(true);
    open->setFont(bold);

    menu->addSeparator();

    // Profiles of every online output; the Windows default output goes first
    QList<const AudioOutputData*> outputs;
    for (const auto& output : mixer->outputsData()) {
        if (!output.Online)
            continue;
        if (output.IsDefault)
            outputs.prepend(&output);
        else
            outputs.append(&output);
    }

    if (outputs.isEmpty())
        menu->addAction(QStringLiteral("No audio outputs"))->setEnabled(false);

    for (const auto* output : std::as_const(outputs)) {
        const int active = output->activeProfileIndex();

        // "Output name — active profile", so the state is readable without opening it
        QString title = output->OutputName;
        if (active >= 0)
            title += QStringLiteral("  —  ") + output->Profiles.at(active).Name;

        auto* submenu = createMenu(menu);
        submenu->setTitle(title);
        submenu->setEnabled(!mixer->editMode());
        submenus.append(submenu);

        auto* submenuAction = menu->addMenu(submenu);
        if (output->IsDefault) {
            QFont font = submenuAction->font();
            font.setBold(true);
            submenuAction->setFont(font);
        }

        auto* group = new QActionGroup(submenu);
        group->setExclusive(true);

        for (int i = 0; i < output->Profiles.size(); ++i) {
            const auto& profile = output->Profiles.at(i);
            QString text = profile.Name;
            if (!profile.Hotkey.isEmpty())
                text += QLatin1Char('\t') + profile.Hotkey.join(QLatin1Char('+'));

            auto* action = submenu->addAction(text);
            action->setCheckable(true);
            action->setChecked(i == active);
            group->addAction(action);

            const QString outputId = output->OutputId;
            const QString profileId = profile.Id;
            connect(action, &QAction::triggered, this, [this, outputId, profileId]() {
                mixer->activateProfileById(outputId, profileId);
            });
        }
    }

    if (mixer->editMode())
        menu->addAction(QStringLiteral("Finish editing to switch profiles"))->setEnabled(false);

    menu->addSeparator();
    menu->addAction(QStringLiteral("Quit"), qApp, &QCoreApplication::quit);
}

void TrayManager::updateToolTip()
{
    QString text = QStringLiteral("Audio Profile Switcher");
    for (const auto& output : mixer->outputsData()) {
        if (!output.IsDefault)
            continue;
        const int active = output.activeProfileIndex();
        if (active >= 0)
            text += QStringLiteral("\n%1: %2").arg(output.OutputName, output.Profiles.at(active).Name);
        break;
    }
    tray->setToolTip(text);
}
