#pragma once

#include <QMainWindow>

namespace Evernus
{
    class MainWindow
        : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
        virtual ~MainWindow() = default;

        void showAsSaved();

    public slots:
        void showAbout();

    protected:
        virtual void closeEvent(QCloseEvent *event) override;

    private:
        static const QString settingsMaximizedKey;
        static const QString settingsPosKey;
        static const QString settingsSizeKey;

        bool mShowMaximized = false;

        void readSettings();
        void writeSettings();

        void createMenu();
    };
}
