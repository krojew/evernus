/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdexcept>

#include <QSplashScreen>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>

#include "ItemPriceImporterNames.h"
#include "ImportSettings.h"
#include "DatabaseUtils.h"

#include "EvernusApplication.h"

namespace Evernus
{
    const QString EvernusApplication::versionKey = "version";

    EvernusApplication::EvernusApplication(int &argc, char *argv[])
        : QApplication{argc, argv}
        , EveDataProvider{}
        , ItemPriceImporterRegistry{}
        , AssetProvider{}
        , mMainDb{QSqlDatabase::addDatabase("QSQLITE", "main")}
        , mEveDb{QSqlDatabase::addDatabase("QSQLITE", "eve")}
    {
        QSplashScreen splash{QPixmap{":/images/splash.png"}};
        splash.show();
        showSplashMessage(tr("Loading..."), splash);

        showSplashMessage(tr("Creating databases..."), splash);

        createDb();

        showSplashMessage(tr("Creating schemas..."), splash);

        createDbSchema();

        showSplashMessage(tr("Loading..."), splash);

        QSettings settings;
        settings.setValue(versionKey, applicationVersion());

        connect(&mAPIManager, &APIManager::generalError, this, &EvernusApplication::apiError);
    }

    QString EvernusApplication::getTypeName(EveType::IdType id) const
    {
        const auto it = mTypeNameCache.find(id);
        if (it != std::end(mTypeNameCache))
            return it->second;

        QString result;

        try
        {
            result = mEveTypeRepository->find(id).getName();
        }
        catch (const EveTypeRepository::NotFoundException &)
        {
        }

        mTypeNameCache.emplace(id, result);
        return result;
    }

    double EvernusApplication::getTypeVolume(EveType::IdType id) const
    {
        const auto it = mTypeVolumeCache.find(id);
        if (it != std::end(mTypeVolumeCache))
            return it->second;

        auto result = 0.;

        try
        {
            result = mEveTypeRepository->find(id).getVolume();
        }
        catch (const EveTypeRepository::NotFoundException &)
        {
        }

        mTypeVolumeCache.emplace(id, result);
        return result;
    }

    ItemPrice EvernusApplication::getTypeSellPrice(EveType::IdType id, quint64 stationId) const
    {
        const auto key = std::make_pair(id, stationId);
        const auto it = mSellPrices.find(key);
        if (it != std::end(mSellPrices))
            return it->second;

        ItemPrice result;

        try
        {
            result = mItemPriceRepository->findSellByTypeAndLocation(id, stationId);
        }
        catch (const ItemPriceRepository::NotFoundException &)
        {
        }

        mSellPrices.emplace(key, result);
        return result;
    }

    QString EvernusApplication::getLocationName(quint64 id) const
    {
        const auto it = mLocationNameCache.find(id);
        if (it != std::end(mLocationNameCache))
            return it->second;

        QString result;
        if (id >= 66000000 && id <= 66014933)
        {
            QSqlQuery query{"SELECT stationName FROM staStations WHERE stationID = ?", mEveDb};
            query.bindValue(0, id - 6000001);

            DatabaseUtils::execQuery(query);
            query.next();

            result = query.value(0).toString();
        }
        else if (id >= 66014934 && id <= 67999999)
        {
            try
            {
                auto station = mConquerableStationRepository->find(id - 6000000);
                result = station.getName();
            }
            catch (const ConquerableStationRepository::NotFoundException &)
            {
            }
        }
        else if (id >= 60014861 && id <= 60014928)
        {
            try
            {
                auto station = mConquerableStationRepository->find(id);
                result = station.getName();
            }
            catch (const ConquerableStationRepository::NotFoundException &)
            {
            }
        }
        else if (id > 60000000 && id <= 61000000)
        {
            QSqlQuery query{"SELECT stationName FROM staStations WHERE stationID = ?", mEveDb};
            query.bindValue(0, id);

            DatabaseUtils::execQuery(query);
            query.next();

            result = query.value(0).toString();
        }
        else if (id > 61000000)
        {
            try
            {
                auto station = mConquerableStationRepository->find(id);
                result = station.getName();
            }
            catch (const ConquerableStationRepository::NotFoundException &)
            {
            }
        }
        else
        {
            QSqlQuery query{"SELECT itemName FROM staStations WHERE itemID = ?", mEveDb};
            query.bindValue(0, id);

            DatabaseUtils::execQuery(query);
            query.next();

            result = query.value(0).toString();
        }

        mLocationNameCache.emplace(id, result);
        return result;
    }

    void EvernusApplication::registerImporter(const std::string &name, std::unique_ptr<ItemPriceImporter> &&importer)
    {
        connect(importer.get(), &ItemPriceImporter::error, this, &EvernusApplication::showPriceImportError);
        connect(importer.get(), &ItemPriceImporter::itemPricesChanged, this, &EvernusApplication::updateItemPrices);
        mItemPriceImporters.emplace(name, std::move(importer));
    }

    const AssetList &EvernusApplication::fetchForCharacter(Character::IdType id) const
    {
        const auto it = mCharacterAssets.find(id);
        if (it != std::end(mCharacterAssets))
            return *it->second;

        std::unique_ptr<AssetList> assets;

        try
        {
            assets = std::make_unique<AssetList>(mAssetListRepository->fetchForCharacter(id));
        }
        catch (const AssetListRepository &)
        {
            assets = std::make_unique<AssetList>();
        }

        const auto assetPtr = assets.get();

        mCharacterAssets.emplace(id, std::move(assets));
        return *assetPtr;
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

    void EvernusApplication::refreshCharacter(Character::IdType id, uint parentTask)
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

    void EvernusApplication::refreshAssets(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing assets: " << id;

        try
        {
            const auto key = getCharacterKey(id);
            const auto assetSubtask = startTask(parentTask, QString{tr("Fetching assets for character %1...")}.arg(id));

            mAPIManager.fetchAssets(key, id, [assetSubtask, id, this](auto data, const auto &error) {
                auto query = mAssetListRepository->prepare(QString{"DELETE FROM %1 WHERE character_id = ?"}
                    .arg(mAssetListRepository->getTableName()));
                query.bindValue(0, id);

                Evernus::DatabaseUtils::execQuery(query);

                mCharacterAssets.erase(id);
                mAssetListRepository->store(data);

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
            mConquerableStationRepository->exec(QString{"DELETE FROM %1"}.arg(mConquerableStationRepository->getTableName()));
            mConquerableStationRepository->batchStore(list);

            emit conquerableStationsChanged();
            emit taskStatusChanged(task, error);
        });
    }

    void EvernusApplication::refreshItemPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &target)
    {
        importItemPrices(ItemPriceImporterNames::webImporter, target);
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

    void EvernusApplication::showPriceImportError(const QString &info)
    {
        Q_ASSERT(mCurrentItemPriceImportTask != TaskConstants::invalidTask);
        finishItemPriceImportTask(info);
    }

    void EvernusApplication::updateItemPrices(const std::vector<ItemPrice> &prices)
    {
        auto query = mItemPriceRepository->prepare(QString{"DELETE FROM %1 WHERE type = :type AND type_id = :type_id AND location_id = :location_id"}
            .arg(mItemPriceRepository->getTableName()));

        try
        {
            for (auto price : prices)
            {
                query.bindValue(":type", static_cast<int>(price.getType()));
                query.bindValue(":type_id", price.getTypeId());
                query.bindValue(":location_id", price.getTypeId());

                DatabaseUtils::execQuery(query);
            }

            mItemPriceRepository->batchStore(prices);

            finishItemPriceImportTask(QString{});
        }
        catch (const std::exception &e)
        {
            finishItemPriceImportTask(e.what());
        }

        mSellPrices.clear();

        emit itemPricesChanged();
    }

    void EvernusApplication::createDb()
    {
        DatabaseUtils::createDb(mMainDb, "main.db");

        if (!mEveDb.isValid())
            throw std::runtime_error{"Error crating Eve DB object!"};

        mEveDb.setDatabaseName(applicationDirPath() + "/res/eve.db");
        mEveDb.setConnectOptions("QSQLITE_OPEN_READONLY");

        if (!mEveDb.open())
            throw std::runtime_error{"Error opening Eve DB!"};

        mKeyRepository.reset(new KeyRepository{mMainDb});
        mCharacterRepository.reset(new CharacterRepository{mMainDb});
        mItemRepository.reset(new ItemRepository{mMainDb});
        mAssetListRepository.reset(new AssetListRepository{mMainDb, *mItemRepository});
        mConquerableStationRepository.reset(new ConquerableStationRepository{mMainDb});
        mWalletSnapshotRepository.reset(new WalletSnapshotRepository{mMainDb});
        mItemPriceRepository.reset(new ItemPriceRepository{mMainDb});
        mEveTypeRepository.reset(new EveTypeRepository{mEveDb});
    }

    void EvernusApplication::createDbSchema()
    {
        mKeyRepository->create();
        mCharacterRepository->create(*mKeyRepository);
        mAssetListRepository->create(*mCharacterRepository);
        mItemRepository->create(*mAssetListRepository);
        mConquerableStationRepository->create();
        mWalletSnapshotRepository->create(*mCharacterRepository);
        mItemPriceRepository->create();
    }

    uint EvernusApplication::startTask(const QString &description)
    {
        emit taskStarted(mTaskId, description);
        return mTaskId++;
    }

    uint EvernusApplication::startTask(uint parentTask, const QString &description)
    {
        if (parentTask == TaskConstants::invalidTask)
            return startTask(description);

        emit taskStarted(mTaskId, parentTask, description);
        return mTaskId++;
    }

    void EvernusApplication::importCharacter(Character::IdType id, uint parentTask, const Key &key)
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

                Evernus::WalletSnapshot snapshot{QDateTime::currentDateTimeUtc(), data.getISK()};
                snapshot.setCharacterId(data.getId());
                mWalletSnapshotRepository->store(snapshot);

                QMetaObject::invokeMethod(this, "scheduleCharacterUpdate", Qt::QueuedConnection);
            }

            emit taskStatusChanged(charSubtask, error);
        });
    }

    void EvernusApplication::importItemPrices(const std::string &importerName, const ItemPriceImporter::TypeLocationPairs &target)
    {
        if (mCurrentItemPriceImportTask != TaskConstants::invalidTask)
            return;

        qDebug() << "Refreshing item prices using importer:" << importerName.c_str();

        const auto it = mItemPriceImporters.find(importerName);
        Q_ASSERT(it != std::end(mItemPriceImporters));

        mCurrentItemPriceImportTask = startTask(tr("Importing item prices..."));

        it->second->fetchItemPrices(target);
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

    void EvernusApplication::finishItemPriceImportTask(const QString &info)
    {
        const auto task = mCurrentItemPriceImportTask;
        mCurrentItemPriceImportTask = TaskConstants::invalidTask;

        emit taskStatusChanged(task, info);
    }

    void EvernusApplication::showSplashMessage(const QString &message, QSplashScreen &splash)
    {
        splash.showMessage(message, Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
        processEvents();
    }
}
