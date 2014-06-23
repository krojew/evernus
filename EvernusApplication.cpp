#include <stdexcept>

#include <QSplashScreen>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>

#include "DatabaseUtils.h"

#include "EvernusApplication.h"

namespace Evernus
{
    const QString EvernusApplication::versionKey = "version";

    EvernusApplication::EvernusApplication(int &argc, char *argv[])
        : QApplication{argc, argv}
        , mMainDb{QSqlDatabase::addDatabase("QSQLITE", "main")}
    {
        QSplashScreen splash{QPixmap{":/images/splash.png"}};
        splash.show();
        showSplashMessage(tr("Loading..."), splash);

        try
        {
            showSplashMessage(tr("Creating databases..."), splash);

            createDb();

            showSplashMessage(tr("Creating schemas..."), splash);

            createDbSchema();

            showSplashMessage(tr("Loading..."), splash);

            QSettings settings;
            settings.setValue(versionKey, applicationVersion());
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
            const auto charListSubtask = startTask(task, QString{tr("Fetching characters for key %1...")}.arg(key.getId()));
            mAPIManager.fetchCharacterList(key, [key, charListSubtask, this](const auto &characters, const auto &error) {
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

                            auto query = mCharacterRepository->prepare(QString{"UPDATE %1 SET key_id = NULL, enabled = 0 WHERE %2 NOT IN (%3)"}
                                .arg(mCharacterRepository->getTableName())
                                .arg(mCharacterRepository->getIdColumn())
                                .arg(ids.join(", ")));

                            for (auto i = 0; i < characters.size(); ++i)
                                query.bindValue(i, characters[i]);

                            query.exec();
                        }
                    }
                    catch (const std::exception &e)
                    {
                        QMessageBox::warning(activeWindow(), tr("Evernus"), tr("An error occurred while updating character key information: %1. "
                            "Data sync should work, but character tab will display incorrect information.").arg(e.what()));
                        return;
                    }
                    catch (...)
                    {
                        QMessageBox::warning(activeWindow(), tr("Evernus"), tr("An error occurred while updating character key information. "
                            "Data sync should work, but character tab will display incorrect information."));
                        return;
                    }

                    for (const auto id : characters)
                    {
                        const auto charSubtask = startTask(charListSubtask, QString{tr("Fetching character %1...")}.arg(id));
                        mAPIManager.fetchCharacter(key, id, [charSubtask, this](const auto &data, const auto &error) {

                            emit taskStatusChanged(charSubtask, error);
                        });
                    }
                }
            });
        }
    }

    void EvernusApplication::createDb()
    {
        DatabaseUtils::createDb(mMainDb, "main.db");

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

    void EvernusApplication::showSplashMessage(const QString &message, QSplashScreen &splash)
    {
        splash.showMessage(message, Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
        processEvents();
    }
}
