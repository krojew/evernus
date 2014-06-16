#include <QCloseEvent>
#include <QSettings>

#include "MainWindow.h"

namespace Evernus
{
    const QString MainWindow::settingsMaximizedKey = "mainWindow/maximized";
    const QString MainWindow::settingsPosKey = "mainWindow/pos";
    const QString MainWindow::settingsSizeKey = "mainWindow/size";

    MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
        : QMainWindow{parent, flags}
    {
        readSettings();
    }

    void MainWindow::showAsSaved()
    {
        if (mShowMaximized)
            showMaximized();
        else
            show();
    }

    void MainWindow::closeEvent(QCloseEvent *event)
    {
        writeSettings();
        event->accept();
    }

    void MainWindow::readSettings()
    {
        QSettings settings;

        const auto pos = settings.value(settingsPosKey).toPoint();
        const auto size = settings.value(settingsSizeKey, QSize{600, 400}).toSize();

        resize(size);
        move(pos);

        mShowMaximized = settings.value(settingsMaximizedKey, false).toBool();
    }

    void MainWindow::writeSettings()
    {
        QSettings settings;
        settings.setValue(settingsPosKey, pos());
        settings.setValue(settingsSizeKey, size());
        settings.setValue(settingsMaximizedKey, isMaximized());
    }
}
