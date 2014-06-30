#include <stdexcept>

#include <QSplashScreen>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>

#include "ImportSettings.h"
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

    void EvernusApplication::refreshCharacters()
    {
        qDebug() << "Refreshing characters...";

        const auto task = startTask(tr("Fetching characters..."));

        const auto keys = mKeyRepository->fetchAll();

        if (keys.empty())
        {
            mCharacterRepository->exec(QString{"UPDATE %1 SET key_id = NULL, enabled = 0"}.arg(mCharacterRepository->getTableName()));
            emit taskStatusChanged(task, QString{});
        }
        else
        {
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
                                    QString{"UPDATE %1 SET key_id = NULL, enabled = 0 WHERE key_id = ?"}.arg(mCharacterRepository->getTableName()));
                                query.bindValue(0, key.getId());
                                query.exec();
                            }
                            else
                            {
                                QStringList ids;
                                for (auto i = 0; i < characters.size(); ++i)
                                    ids << "?";

                                auto query = mCharacterRepository->prepare(QString{"UPDATE %1 SET key_id = NULL, enabled = 0 WHERE key_id = ? AND %2 NOT IN (%3)"}
                                    .arg(mCharacterRepository->getTableName())
                                    .arg(mCharacterRepository->getIdColumn())
                                    .arg(ids.join(", ")));

                                query.bindValue(0, key.getId());

                                for (auto i = 0; i < characters.size(); ++i)
                                    query.bindValue(i + 1, characters[i]);

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

                        if (characters.empty())
                        {
                            emit taskStatusChanged(charListSubtask, error);
                        }
                        else
                        {
                            for (const auto id : characters)
                                importCharacter(id, charListSubtask, key);
                        }
                    }
                    else
                    {
                        emit taskStatusChanged(charListSubtask, error);
                    }
                });
            }
        }
    }

    void EvernusApplication::refreshCharacter(Character::IdType id, quint32 parentTask)
    {
        qDebug() << "Refreshing character: " << id;

        try
        {
            importCharacter(id, parentTask, getCharacterKey(id));
        }
        catch (const KeyRepository::NotFoundException &)
        {
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }
    }

    void EvernusApplication::refreshAssets(Character::IdType id, quint32 parentTask)
    {
        qDebug() << "Refreshing assets: " << id;

        try
        {
            const auto key = getCharacterKey(id);
            const auto assetSubtask = startTask(parentTask, QString{tr("Fetching assets for character %1...")}.arg(id));

            mAPIManager.fetchAssets(key, id, [assetSubtask, this](auto data, const auto &error) {
                emit assetsChanged();
                emit taskStatusChanged(assetSubtask, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }
    }

    void EvernusApplication::refreshConquerableStations()
    {
        qDebug() << "Refreshing conquerable stations...";

        const auto task = startTask(tr("Fetching conquerable stations..."));

        mAPIManager.fetchConquerableStationList([task, this](const auto &list, const auto &error) {
            emit taskStatusChanged(task, error);
        });
    }

    void EvernusApplication::scheduleCharacterUpdate()
    {
        if (mCharacterUpdateScheduled)
            return;

        mCharacterUpdateScheduled = true;
        QMetaObject::invokeMethod(this, "updateCharacters", Qt::QueuedConnection);
    }

    void EvernusApplication::updateCharacters()
    {
        mCharacterUpdateScheduled = false;

        emit charactersChanged();
        emit iskChanged();
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
        if (parentTask == TaskConstants::invalidTask)
            return startTask(description);

        emit taskStarted(mTaskId, parentTask, description);
        return mTaskId++;
    }

    void EvernusApplication::importCharacter(Character::IdType id, quint32 parentTask, const Key &key)
    {
        const auto charSubtask = startTask(parentTask, QString{tr("Fetching character %1...")}.arg(id));
        mAPIManager.fetchCharacter(key, id, [charSubtask, this](auto data, const auto &error) {
            if (error.isEmpty())
            {
                QSettings settings;
                if (!settings.value(Evernus::ImportSettings::importSkillsKey, true).toBool())
                {
                    try
                    {
                        const auto prevData = mCharacterRepository->find(data.getId());
                        data.setOrderAmountSkills(prevData.getOrderAmountSkills());
                        data.setTradeRangeSkills(prevData.getTradeRangeSkills());
                        data.setFeeSkills(prevData.getFeeSkills());
                        data.setContractSkills(prevData.getContractSkills());
                    }
                    catch (const Evernus::CharacterRepository::NotFoundException &)
                    {
                    }
                }

                mCharacterRepository->store(data);
                QMetaObject::invokeMethod(this, "scheduleCharacterUpdate", Qt::QueuedConnection);
            }

            emit taskStatusChanged(charSubtask, error);
        });
    }

    Key EvernusApplication::getCharacterKey(Character::IdType id) const
    {
        try
        {
            const auto character = mCharacterRepository->find(id);
            const auto keyId = character.getKeyId();

            if (keyId)
            {
                try
                {
                    return mKeyRepository->find(*keyId);
                }
                catch (const KeyRepository::NotFoundException &)
                {
                    qCritical() << "Attempted to refresh character without a key!";
                    throw;
                }
            }
            else
            {
                throw KeyRepository::NotFoundException{};
            }
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            qCritical() << "Attempted to refresh non-existent character!";
            throw;
        }
    }

    void EvernusApplication::showSplashMessage(const QString &message, QSplashScreen &splash)
    {
        splash.showMessage(message, Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
        processEvents();
    }
}
