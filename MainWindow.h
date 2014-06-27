#pragma once

#include <QMainWindow>

#include "Character.h"

class QLabel;

namespace Evernus
{
    template<class T>
    class Repository;
    class CharacterManagerDialog;
    class ActiveTasksDialog;
    class MenuBarWidget;
    class APIManager;
    class Key;

    class MainWindow
        : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(const Repository<Character> &characterRepository,
                   const Repository<Key> &keyRepository,
                   APIManager &apiManager,
                   QWidget *parent = nullptr,
                   Qt::WindowFlags flags = 0);
        virtual ~MainWindow() = default;

        void showAsSaved();

    signals:
        void refreshCharacters();
        void charactersChanged();

        void newTaskInfoAdded(quint32 taskId, const QString &description);
        void newSubTaskInfoAdded(quint32 taskId, quint32 parentTask, const QString &description);
        void taskStatusChanged(quint32 taskId, const QString &error);

    public slots:
        void showCharacterManagement();
        void showPreferences();
        void showAbout();
        void showError(const QString &info);

        void addNewTaskInfo(quint32 taskId, const QString &description);

        void updateStatus();

        void setCharacter(Character::IdType id);

    protected:
        virtual void closeEvent(QCloseEvent *event) override;

    private:
        static const QString settingsMaximizedKey;
        static const QString settingsPosKey;
        static const QString settingsSizeKey;

        const Repository<Character> &mCharacterRepository;
        const Repository<Key> &mKeyRepository;

        MenuBarWidget *mMenuWidget = nullptr;

        APIManager &mApiManager;

        bool mShowMaximized = false;

        CharacterManagerDialog *mCharacterManagerDialog = nullptr;
        ActiveTasksDialog *mActiveTasksDialog = nullptr;

        QLabel *mStatusWalletLabel = nullptr;

        Character::IdType mCurrentCharacterId = Character::invalidId;

        void readSettings();
        void writeSettings();

        void createMenu();
        void createMainView();
        void createStatusBar();
    };
}
