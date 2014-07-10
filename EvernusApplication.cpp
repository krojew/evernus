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
#include <QSet>

#include "ItemPriceImporterNames.h"
#include "ImportSettings.h"
#include "WalletSettings.h"
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
        , CacheTimerProvider{}
        , mMainDb{QSqlDatabase::addDatabase("QSQLITE", "main")}
        , mEveDb{QSqlDatabase::addDatabase("QSQLITE", "eve")}
        , mAPIManager{*this}
    {
        QSplashScreen splash{QPixmap{":/images/splash.png"}};
        splash.show();
        showSplashMessage(tr("Loading..."), splash);

        showSplashMessage(tr("Creating databases..."), splash);
        createDb();

        showSplashMessage(tr("Creating schemas..."), splash);
        createDbSchema();

        showSplashMessage(tr("Precaching ref types..."), splash);
        precacheRefTypes();

        showSplashMessage(tr("Precaching cache timers..."), splash);
        precacheCacheTimers();

        showSplashMessage(tr("Clearing old wallet entries..."), splash);
        deleteOldWalletEntries();

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
        return getTypeSellPrice(id, stationId, true);
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

    QString EvernusApplication::getRefTypeName(uint id) const
    {
        const auto it = mRefTypeNames.find(id);
        return (it != std::end(mRefTypeNames)) ? (it->second) : (QString{});
    }

    void EvernusApplication::registerImporter(const std::string &name, std::unique_ptr<ItemPriceImporter> &&importer)
    {
        Q_ASSERT(mItemPriceImporters.find(name) == std::end(mItemPriceImporters));

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
            assets->setCharacterId(id);
        }

        const auto assetPtr = assets.get();

        mCharacterAssets.emplace(id, std::move(assets));
        return *assetPtr;
    }

    QDateTime EvernusApplication::getLocalCacheTimer(Character::IdType id, TimerType type) const
    {
        CacheTimerMap::const_iterator it;

        switch (type) {
        case TimerType::Character:
            it = mCharacterLocalCacheTimes.find(id);
            if (it == std::end(mCharacterLocalCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::AssetList:
            it = mAssetsLocalCacheTimes.find(id);
            if (it == std::end(mAssetsLocalCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::WalletJournal:
            it = mWalletJournalLocalCacheTimes.find(id);
            if (it == std::end(mWalletJournalLocalCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::WalletTransactions:
            it = mWalletTransactionsLocalCacheTimes.find(id);
            if (it == std::end(mWalletTransactionsLocalCacheTimes))
                return QDateTime::currentDateTime();
            break;
        default:
            throw std::logic_error{tr("Unknown cache timer type: %1").arg(static_cast<int>(type)).toStdString()};
        }

        return it->second.toLocalTime();
    }

    void EvernusApplication::setUtcCacheTimer(Character::IdType id, TimerType type, const QDateTime &dt)
    {
        if (!dt.isValid())
            return;

        switch (type) {
        case TimerType::Character:
            mCharacterLocalCacheTimes[id] = dt;
            break;
        case TimerType::AssetList:
            mAssetsLocalCacheTimes[id] = dt;
            break;
        case TimerType::WalletJournal:
            mWalletJournalLocalCacheTimes[id] = dt;
            break;
        case TimerType::WalletTransactions:
            mWalletTransactionsLocalCacheTimes[id] = dt;
            break;
        default:
            throw std::logic_error{tr("Unknown cache timer type: %1").arg(static_cast<int>(type)).toStdString()};
        }

        CacheTimer timer{static_cast<CacheTimer::IdType>(type), dt};
        timer.setCharacterId(id);

        mCacheTimerRepository->store(timer);
    }

    const KeyRepository &EvernusApplication::getKeyRepository() const noexcept
    {
        return *mKeyRepository;
    }

    const CharacterRepository &EvernusApplication::getCharacterRepository() const noexcept
    {
        return *mCharacterRepository;
    }

    const WalletSnapshotRepository &EvernusApplication::getWalletSnapshotRepository() const noexcept
    {
        return *mWalletSnapshotRepository;
    }

    const AssetValueSnapshotRepository &EvernusApplication::getAssetValueSnapshotRepository() const noexcept
    {
        return *mAssetValueSnapshotRepository;
    }

    const WalletJournalEntryRepository &EvernusApplication::getWalletJournalEntryRepository() const noexcept
    {
        return *mWalletJournalEntryRepository;
    }

    const WalletTransactionRepository &EvernusApplication::getWalletTransactionRepository() const noexcept
    {
        return *mWalletTransactionRepository;
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
                const auto charListSubtask = startTask(task, tr("Fetching characters for key %1...").arg(key.getId()));
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

        const auto assetSubtask = startTask(parentTask, tr("Fetching assets for character %1...").arg(id));
        processEvents();

        try
        {
            const auto key = getCharacterKey(id);

            mAPIManager.fetchAssets(key, id, [assetSubtask, id, this](auto data, const auto &error) {
                auto query = mAssetListRepository->prepare(QString{"DELETE FROM %1 WHERE character_id = ?"}
                    .arg(mAssetListRepository->getTableName()));
                query.bindValue(0, id);

                Evernus::DatabaseUtils::execQuery(query);

                mCharacterAssets.erase(id);
                mAssetListRepository->store(data);

                QSettings settings;

                if (settings.value(Evernus::ImportSettings::autoUpdateAssetValueKey, true).toBool())
                    computeAssetListSellValue(data);

                emit assetsChanged();
                emit taskStatusChanged(assetSubtask, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskStatusChanged(assetSubtask, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskStatusChanged(assetSubtask, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshWalletJournal(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing wallet journal: " << id;

        const auto task = startTask(tr("Fetching wallet journal for character %1...").arg(id));
        processEvents();

        try
        {
            const auto key = getCharacterKey(id);
            const auto maxId = mWalletJournalEntryRepository->getLatestEntryId(id);

            mAPIManager.fetchWalletJournal(key, id, WalletJournalEntry::invalidId, maxId,
                                           [task, this](const auto &data, const auto &error) {
                std::vector<Evernus::WalletJournalEntry> vectorData;
                vectorData.reserve(data.size());

                std::vector<Evernus::WalletSnapshot> snapshots;
                snapshots.reserve(data.size());

                QSet<QDateTime> usedSnapshots;

                for (auto &entry : data)
                {
                    const auto timestamp = entry.getTimestamp();

                    if (!usedSnapshots.contains(timestamp))
                    {
                        Evernus::WalletSnapshot snapshot{timestamp, entry.getBalance()};
                        snapshot.setCharacterId(entry.getCharacterId());

                        snapshots.emplace_back(std::move(snapshot));
                        usedSnapshots << timestamp;
                    }
                    
                    vectorData.emplace_back(std::move(entry));
                }

                mWalletJournalEntryRepository->batchStore(vectorData, true);
                mWalletSnapshotRepository->batchStore(snapshots, true);

                emit walletJournalChanged();
                emit taskStatusChanged(task, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskStatusChanged(task, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskStatusChanged(task, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshWalletTransactions(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing wallet transactions: " << id;
    }

    void EvernusApplication::refreshConquerableStations()
    {
        qDebug() << "Refreshing conquerable stations...";

        const auto task = startTask(tr("Fetching conquerable stations..."));
        processEvents();

        mAPIManager.fetchConquerableStationList([task, this](const auto &list, const auto &error) {
            mConquerableStationRepository->exec(QString{"DELETE FROM %1"}.arg(mConquerableStationRepository->getTableName()));
            mConquerableStationRepository->batchStore(list, true);

            emit conquerableStationsChanged();
            emit taskStatusChanged(task, error);
        });
    }

    void EvernusApplication::refreshItemPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &target)
    {
        importItemPrices(ItemPriceImporterNames::webImporter, target);
    }

    void EvernusApplication::refreshItemPricesFromFile(const ItemPriceImporter::TypeLocationPairs &target)
    {
        importItemPrices(ItemPriceImporterNames::logImporter, target);
    }

    void EvernusApplication::refreshItemPricesFromCache(const ItemPriceImporter::TypeLocationPairs &target)
    {
        importItemPrices(ItemPriceImporterNames::cacheImporter, target);
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
        try
        {
            mItemPriceRepository->batchStore(prices, false);

            QSettings settings;
            if (settings.value(ImportSettings::autoUpdateAssetValueKey, true).toBool())
            {
                for (const auto &list : mCharacterAssets)
                    computeAssetListSellValue(*list.second);
            }

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
        mAssetValueSnapshotRepository.reset(new AssetValueSnapshotRepository{mMainDb});
        mWalletJournalEntryRepository.reset(new WalletJournalEntryRepository{mMainDb});
        mRefTypeRepository.reset(new RefTypeRepository{mMainDb});
        mCacheTimerRepository.reset(new CacheTimerRepository{mMainDb});
        mWalletTransactionRepository.reset(new WalletTransactionRepository{mMainDb});
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
        mAssetValueSnapshotRepository->create(*mCharacterRepository);
        mWalletJournalEntryRepository->create(*mCharacterRepository);
        mItemPriceRepository->create();
        mCacheTimerRepository->create(*mCharacterRepository);
        mWalletTransactionRepository->create(*mCharacterRepository);
        mRefTypeRepository->create();
    }

    void EvernusApplication::precacheRefTypes()
    {
        const auto refs = mRefTypeRepository->fetchAll();
        if (refs.empty())
        {
            qDebug() << "Fetching ref types...";
            mAPIManager.fetchRefTypes([this](const auto &refs, const auto &error) {
                if (error.isEmpty())
                {
                    mRefTypeRepository->batchStore(refs, true);
                    precacheRefTypes(refs);
                }
                else
                {
                    qDebug() << error;
                }
            });
        }
        else
        {
            precacheRefTypes(refs);
        }
    }

    void EvernusApplication::precacheRefTypes(const RefTypeRepository::RefTypeList &refs)
    {
        for (const auto &ref : refs)
            mRefTypeNames.emplace(ref.getId(), std::move(ref).getName());
    }

    void EvernusApplication::precacheCacheTimers()
    {
        const auto timers = mCacheTimerRepository->fetchAll();
        for (const auto &timer : timers)
        {
            switch (static_cast<TimerType>(timer.getId())) {
            case TimerType::Character:
                mCharacterLocalCacheTimes[timer.getCharacterId()] = timer.getCacheUntil();
                break;
            case TimerType::AssetList:
                mAssetsLocalCacheTimes[timer.getCharacterId()] = timer.getCacheUntil();
                break;
            case TimerType::WalletJournal:
                mWalletJournalLocalCacheTimes[timer.getCharacterId()] = timer.getCacheUntil();
                break;
            case TimerType::WalletTransactions:
                mWalletTransactionsLocalCacheTimes[timer.getCharacterId()] = timer.getCacheUntil();
            }
        }
    }

    void EvernusApplication::deleteOldWalletEntries()
    {
        QSettings settings;

        const auto dt = QDateTime::currentDateTimeUtc().addDays(
             -settings.value(WalletSettings::oldJournalDaysKey, WalletSettings::oldJournalDaysDefault).toInt());
        mWalletJournalEntryRepository->deleteOldEntires(dt);
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
        const auto charSubtask = startTask(parentTask, tr("Fetching character %1...").arg(id));
        mAPIManager.fetchCharacter(key, id, [charSubtask, this](auto data, const auto &error) {
            if (error.isEmpty())
            {
                try
                {
                    const auto prevData = mCharacterRepository->find(data.getId());

                    QSettings settings;
                    if (!settings.value(Evernus::ImportSettings::importSkillsKey, true).toBool())
                    {
                        data.setOrderAmountSkills(prevData.getOrderAmountSkills());
                        data.setTradeRangeSkills(prevData.getTradeRangeSkills());
                        data.setFeeSkills(prevData.getFeeSkills());
                        data.setContractSkills(prevData.getContractSkills());
                    }

                    data.setEnabled(prevData.isEnabled());
                }
                catch (const Evernus::CharacterRepository::NotFoundException &)
                {
                }

                mMainDb.exec("PRAGMA foreign_keys = OFF;");
                mCharacterRepository->store(data);
                mMainDb.exec("PRAGMA foreign_keys = ON;");

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
        processEvents();

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

    ItemPrice EvernusApplication::getTypeSellPrice(EveType::IdType id, quint64 stationId, bool dontThrow) const
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
            if (!dontThrow)
                throw;
        }

        mSellPrices.emplace(key, result);
        return result;
    }

    void EvernusApplication::computeAssetListSellValue(const AssetList &list) const
    {
        try
        {
            auto value = 0.;
            for (const auto &item : list)
            {
                const auto locationId = item->getLocationId();
                if (!locationId)
                    continue;

                value += getTotalItemSellValue(*item, *locationId);
            }

            AssetValueSnapshot snapshot{QDateTime::currentDateTimeUtc(), value};
            snapshot.setCharacterId(list.getCharacterId());

            mAssetValueSnapshotRepository->store(snapshot);
        }
        catch (const ItemPriceRepository::NotFoundException &)
        {
        }
    }

    double EvernusApplication::getTotalItemSellValue(const Item &item, quint64 locationId) const
    {
        auto price = getTypeSellPrice(item.getTypeId(), locationId, false).getValue() * item.getQuantity();
        for (const auto &child : item)
            price += getTotalItemSellValue(*child, locationId);

        return price;
    }

    void EvernusApplication::showSplashMessage(const QString &message, QSplashScreen &splash)
    {
        splash.showMessage(message, Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
        processEvents();
    }
}
