#pragma once

#include <memory>

#include <QApplication>
#include <QSqlDatabase>

#include "CharacterRepository.h"
#include "TaskConstants.h"
#include "KeyRepository.h"
#include "APIManager.h"

class QSplashScreen;

namespace Evernus
{
    class Key;

    class EvernusApplication
        : public QApplication
    {
        Q_OBJECT

    public:
        EvernusApplication(int &argc, char *argv[]);
        virtual ~EvernusApplication() = default;

        const KeyRepository &getKeyRepository() const noexcept;
        const CharacterRepository &getCharacterRepository() const noexcept;

        APIManager &getAPIManager() noexcept;

    signals:
        void taskStarted(quint32 taskId, const QString &description);
        void taskStarted(quint32 taskId, quint32 parentTask, const QString &description);
        void taskStatusChanged(quint32 taskId, const QString &error);

        void apiError(const QString &info);

        void charactersChanged();
        void iskChanged();

    public slots:
        void fetchCharacters();
        void refreshCharacter(Character::IdType id, quint32 parentTask = TaskConstants::invalidTask);

        void refreshAssets(Character::IdType id, quint32 parentTask = TaskConstants::invalidTask);

    private slots:
        void scheduleCharacterUpdate();
        void updateCharacters();

    private:
        static const QString versionKey;

        QSqlDatabase mMainDb;

        std::unique_ptr<KeyRepository> mKeyRepository;
        std::unique_ptr<CharacterRepository> mCharacterRepository;

        APIManager mAPIManager;

        quint32 mTaskId = TaskConstants::invalidTask + 1;

        bool mCharacterUpdateScheduled = false;

        void createDb();
        void createDbSchema();

        quint32 startTask(const QString &description);
        quint32 startTask(quint32 parentTask, const QString &description);

        void importCharacter(Character::IdType id, quint32 parentTask, const Key &key);

        Key getCharacterKey(Character::IdType id) const;

        static void showSplashMessage(const QString &message, QSplashScreen &splash);
    };
}
