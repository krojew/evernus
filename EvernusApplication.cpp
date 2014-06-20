#include <stdexcept>

#include <QStandardPaths>
#include <QMessageBox>
#include <QDebug>
#include <QDir>

#include "EvernusApplication.h"

namespace Evernus
{
    EvernusApplication::EvernusApplication(int &argc, char *argv[])
        : QApplication{argc, argv}
        , mMainDb{QSqlDatabase::addDatabase("QSQLITE")}
    {
        setApplicationName("Evernus");
        setApplicationVersion("0.1 BETA");
        setOrganizationDomain("evernus.com");
        setOrganizationName("evernus.com");

        try
        {
            createDb();
            createDbSchema();
        }
        catch (const std::exception &e)
        {
            QMessageBox::critical(nullptr, tr("Error"), e.what());
            throw;
        }

        connect(&mAPIManager, &APIManager::generalError, this, &EvernusApplication::apiError);
    }

    const KeyRepository &EvernusApplication::getKeyRepository() const noexcept
    {
        return *mKeyRepository;
    }

    const CharacterRepository &EvernusApplication::getCharacterRepository() const noexcept
    {
        return *mCharacterRepository;
    }

    APIManager &EvernusApplication::getAPIManager() noexcept
    {
        return mAPIManager;
    }

    void EvernusApplication::fetchCharacters()
    {
        qDebug() << "Fetching characters...";

        const auto task = startTask(tr("Fetching characters..."));

        const auto keys = mKeyRepository->fetchAll();
        for (const auto &key : keys)
        {
            const auto subtask = startTask(task, QString{tr("Fetching characters for key %1...")}.arg(key.getId()));
            mAPIManager.fetchCharacterList(key, [key, subtask, this](const APIManager::CharacterList &characters, const QString &error) {
                if (error.isEmpty())
                {
                    try
                    {
                        if (characters.empty())
                        {
                            auto query = mCharacterRepository->prepare(
                                QString{"UPDATE %1 SET key_id = NULL WHERE key_id = ?"}.arg(mCharacterRepository->getTableName()));
                            query.bindValue(0, key.getCode());
                            query.exec();
                        }
                        else
                        {
                            QStringList ids;
                            for (auto i = 0; i < characters.size(); ++i)
                                ids << "?";

                            auto query = mCharacterRepository->prepare(QString{"UPDATE %1 SET key_id = NULL WHERE %2 NOT IN (%3)"}
                                .arg(mCharacterRepository->getTableName())
                                .arg(mCharacterRepository->getIdColumn())
                                .arg(ids.join(", ")));

                            for (auto i = 0; i < characters.size(); ++i)
                                query.bindValue(i, characters[i]);

                            query.exec();
                        }
                    }
                    catch (...)
                    {
                        QMessageBox::warning(activeWindow(), tr("Evernus"), tr("An error occurred updating character key information. "
                            "Data sync should work, but character tab will display incorrect information."));
                        return;
                    }
                }

                emit taskStatusChanged(subtask, error);
            });
        }
    }

    void EvernusApplication::createDb()
    {
        if (!mMainDb.isValid())
            throw std::runtime_error{tr("Error crating DB object!").toStdString()};

        const auto dbPath =
            QStandardPaths::writableLocation(QStandardPaths::DataLocation) +
            QDir::separator() +
            "db" +
            QDir::separator();

        qDebug() << "DB path: " << dbPath;

        if (!QDir{}.mkpath(dbPath))
            throw std::runtime_error{tr("Error creating DB path!").toStdString()};

        mMainDb.setDatabaseName(dbPath + "main.db");
        if (!mMainDb.open())
            throw std::runtime_error{tr("Error opening DB!").toStdString()};

        mMainDb.exec("PRAGMA foreign_keys = ON");

        mKeyRepository.reset(new KeyRepository{mMainDb});
        mCharacterRepository.reset(new CharacterRepository{mMainDb});
    }

    void EvernusApplication::createDbSchema()
    {
        mKeyRepository->create();
        mCharacterRepository->create(*mKeyRepository);
    }

    quint32 EvernusApplication::startTask(const QString &description)
    {
        emit taskStarted(mTaskId, description);
        return mTaskId++;
    }

    quint32 EvernusApplication::startTask(quint32 parentTask, const QString &description)
    {
        emit taskStarted(mTaskId, parentTask, description);
        return mTaskId++;
    }
}
