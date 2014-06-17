#pragma once

#include <QMainWindow>

namespace Evernus
{
    class CharacterManagerDialog;
    class CharacterRepository;
    class KeyRepository;

    class MainWindow
        : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(const CharacterRepository &characterRepository,
                   const KeyRepository &keyRepository,
                   QWidget *parent = nullptr,
                   Qt::WindowFlags flags = 0);
        virtual ~MainWindow() = default;

        void showAsSaved();

    public slots:
        void showCharacterManagement();
        void showAbout();

    protected:
        virtual void closeEvent(QCloseEvent *event) override;

    private:
        static const QString settingsMaximizedKey;
        static const QString settingsPosKey;
        static const QString settingsSizeKey;

        const CharacterRepository &mCharacterRepository;
        const KeyRepository &mKeyRepository;

        bool mShowMaximized = false;

        CharacterManagerDialog *mCharacterManager = nullptr;

        void readSettings();
        void writeSettings();

        void createMenu();
    };
}
