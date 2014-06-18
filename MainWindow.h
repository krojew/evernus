#pragma once

#include <QMainWindow>

namespace Evernus
{
    template<class T>
    class Repository;
    class CharacterManagerDialog;
    class Character;
    class Key;

    class MainWindow
        : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(const Repository<Character> &characterRepository,
                   const Repository<Key> &keyRepository,
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

        const Repository<Character> &mCharacterRepository;
        const Repository<Key> &mKeyRepository;

        bool mShowMaximized = false;

        CharacterManagerDialog *mCharacterManager = nullptr;

        void readSettings();
        void writeSettings();

        void createMenu();
    };
}
