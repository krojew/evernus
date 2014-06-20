#pragma once

#include <memory>

#include <QApplication>
#include <QSqlDatabase>

#include "CharacterRepository.h"
#include "KeyRepository.h"
#include "APIManager.h"

namespace Evernus
{
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
        void taskStatusChanged(quint32 taskId, bool success);

    public slots:
        void fetchCharacters();

    private:
        QSqlDatabase mMainDb;

        std::unique_ptr<KeyRepository> mKeyRepository;
        std::unique_ptr<CharacterRepository> mCharacterRepository;

        APIManager mAPIManager;

        quint32 mTaskId = 0;

        void createDb();
        void createDbSchema();

        quint32 startTask(const QString &description);
        quint32 startTask(quint32 parentTask, const QString &description);
    };
}
