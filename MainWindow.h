#pragma once

#include <QMainWindow>

namespace Evernus
{
    class MainWindow
        : public QMainWindow
    {
    public:
        MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
        virtual ~MainWindow() = default;

        void showAsSaved();

    protected:
        virtual void closeEvent(QCloseEvent *event) override;

    private:
        static const QString settingsMaximizedKey;
        static const QString settingsPosKey;
        static const QString settingsSizeKey;

        bool mShowMaximized = false;

        void readSettings();
        void writeSettings();
    };
}
