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
#include "PathSettings.h"
#include "PathUtils.h"

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
        , MarketOrderProvider{}
        , ItemCostProvider{}
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
        return getEveType(id).getName();
    }

    QString EvernusApplication::getTypeMarketGroupParentName(EveType::IdType id) const
    {
        const auto &type = getEveType(id);
        const auto marketGroupId = type.getMarketGroupId();
        return (marketGroupId) ? (getMarketGroupParent(*marketGroupId).getName()) : (QString{});
    }

    MarketGroup::IdType EvernusApplication::getTypeMarketGroupParentId(EveType::IdType id) const
    {
        const auto &type = getEveType(id);
        const auto marketGroupId = type.getMarketGroupId();
        return (marketGroupId) ? (getMarketGroupParent(*marketGroupId).getId()) : (MarketGroup::invalidId);
    }

    const std::unordered_map<EveType::IdType, QString> &EvernusApplication::getAllTypeNames() const
    {
        if (!mTypeNameCache.empty())
            return mTypeNameCache;

        mTypeNameCache = mEveTypeRepository->fetchAllNames();
        return mTypeNameCache;
    }

    QString EvernusApplication::getTypeMetaGroupName(EveType::IdType id) const
    {
        const auto it = mTypeMetaGroupCache.find(id);
        if (it != std::end(mTypeMetaGroupCache))
            return it->second.getName();

        MetaGroup result;

        try
        {
            result = mMetaGroupRepository->fetchForType(id);
        }
        catch (const MetaGroupRepository::NotFoundException &)
        {
        }

        mTypeMetaGroupCache.emplace(id, result);
        return result.getName();
    }

    double EvernusApplication::getTypeVolume(EveType::IdType id) const
    {
        const auto it = mTypeCache.find(id);
        if (it != std::end(mTypeCache))
            return it->second.getVolume();

        EveType result;

        try
        {
            result = mEveTypeRepository->find(id);
        }
        catch (const EveTypeRepository::NotFoundException &)
        {
        }

        mTypeCache.emplace(id, result);
        return result.getVolume();
    }

    ItemPrice EvernusApplication::getTypeSellPrice(EveType::IdType id, quint64 stationId) const
    {
        return getTypeSellPrice(id, stationId, true);
    }

    ItemPrice EvernusApplication::getTypeBuyPrice(EveType::IdType id, quint64 stationId) const
    {
        return getTypeBuyPrice(id, stationId, true);
    }

    void EvernusApplication::setTypeSellPrice(quint64 stationId,
                                              EveType::IdType typeId,
                                              const QDateTime &priceTime,
                                              double price) const
    {
        mSellPrices[std::make_pair(typeId, stationId)] = saveTypePrice(ItemPrice::Type::Sell, stationId, typeId, priceTime, price);
    }

    void EvernusApplication::setTypeBuyPrice(quint64 stationId,
                                             EveType::IdType typeId,
                                             const QDateTime &priceTime,
                                             double price) const
    {
        mBuyPrices[std::make_pair(typeId, stationId)] = saveTypePrice(ItemPrice::Type::Buy, stationId, typeId, priceTime, price);
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

    const AssetList &EvernusApplication::fetchAssetsForCharacter(Character::IdType id) const
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
        CharacterTimerMap::const_iterator it;

        switch (type) {
        case TimerType::Character:
            it = mCharacterUtcCacheTimes.find(id);
            if (it == std::end(mCharacterUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::AssetList:
            it = mAssetsUtcCacheTimes.find(id);
            if (it == std::end(mAssetsUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::WalletJournal:
            it = mWalletJournalUtcCacheTimes.find(id);
            if (it == std::end(mWalletJournalUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::WalletTransactions:
            it = mWalletTransactionsUtcCacheTimes.find(id);
            if (it == std::end(mWalletTransactionsUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::MarketOrders:
            it = mMarketOrdersUtcCacheTimes.find(id);
            if (it == std::end(mMarketOrdersUtcCacheTimes))
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
            mCharacterUtcCacheTimes[id] = dt;
            break;
        case TimerType::AssetList:
            mAssetsUtcCacheTimes[id] = dt;
            break;
        case TimerType::WalletJournal:
            mWalletJournalUtcCacheTimes[id] = dt;
            break;
        case TimerType::WalletTransactions:
            mWalletTransactionsUtcCacheTimes[id] = dt;
            break;
        case TimerType::MarketOrders:
            mMarketOrdersUtcCacheTimes[id] = dt;
            break;
        default:
            throw std::logic_error{tr("Unknown cache timer type: %1").arg(static_cast<int>(type)).toStdString()};
        }

        CacheTimer timer;
        timer.setCharacterId(id);
        timer.setType(type);
        timer.setCacheUntil(dt);

        mCacheTimerRepository->store(timer);
    }

    QDateTime EvernusApplication::getLocalUpdateTimer(Character::IdType id, TimerType type) const
    {
        CharacterTimerMap::const_iterator it;

        switch (type) {
        case TimerType::Character:
            it = mCharacterUtcUpdateTimes.find(id);
            if (it == std::end(mCharacterUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::AssetList:
            it = mAssetsUtcUpdateTimes.find(id);
            if (it == std::end(mAssetsUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::WalletJournal:
            it = mWalletJournalUtcUpdateTimes.find(id);
            if (it == std::end(mWalletJournalUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::WalletTransactions:
            it = mWalletTransactionsUtcUpdateTimes.find(id);
            if (it == std::end(mWalletTransactionsUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::MarketOrders:
            it = mMarketOrdersUtcUpdateTimes.find(id);
            if (it == std::end(mMarketOrdersUtcUpdateTimes))
                return QDateTime{};
            break;
        default:
            throw std::logic_error{tr("Unknown update timer type: %1").arg(static_cast<int>(type)).toStdString()};
        }

        return it->second.toLocalTime();
    }

    std::vector<MarketOrder> EvernusApplication::getSellOrders(Character::IdType characterId) const
    {
        auto it = mSellOrders.find(characterId);
        if (it == std::end(mSellOrders))
            it = mSellOrders.emplace(characterId, mMarketOrderRepository->fetchForCharacter(characterId, MarketOrder::Type::Sell)).first;

        return it->second;
    }

    std::vector<MarketOrder> EvernusApplication::getBuyOrders(Character::IdType characterId) const
    {
        auto it = mBuyOrders.find(characterId);
        if (it == std::end(mBuyOrders))
            it = mBuyOrders.emplace(characterId, mMarketOrderRepository->fetchForCharacter(characterId, MarketOrder::Type::Buy)).first;

        return it->second;
    }

    std::vector<MarketOrder> EvernusApplication::getArchivedOrders(Character::IdType characterId) const
    {
        auto it = mArchivedOrders.find(characterId);
        if (it == std::end(mArchivedOrders))
            it = mArchivedOrders.emplace(characterId, mMarketOrderRepository->fetchArchivedForCharacter(characterId)).first;

        return it->second;
    }

    ItemCost EvernusApplication::fetchForCharacterAndType(Character::IdType characterId, EveType::IdType typeId) const
    {
        const auto it = mItemCostCache.find(std::make_pair(characterId, typeId));
        if (it != std::end(mItemCostCache))
            return it->second;

        ItemCost cost;

        try
        {
            cost = mItemCostRepository->fetchForCharacterAndType(characterId, typeId);
        }
        catch (const ItemCostRepository::NotFoundException &)
        {
        }

        mItemCostCache.emplace(std::make_pair(characterId, typeId), cost);
        return cost;
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

    const MarketOrderRepository &EvernusApplication::getMarketOrderRepository() const noexcept
    {
        return *mMarketOrderRepository;
    }

    const ItemCostRepository &EvernusApplication::getItemCostRepository() const noexcept
    {
        return *mItemCostRepository;
    }

    const MarketOrderValueSnapshotRepository &EvernusApplication::getMarketOrderValueSnapshotRepository() const noexcept
    {
        return *mMarketOrderValueSnapshotRepository;
    }

    void EvernusApplication::refreshCharacters()
    {
        qDebug() << "Refreshing characters...";

        const auto task = startTask(tr("Fetching characters..."));

        const auto keys = mKeyRepository->fetchAll();

        if (keys.empty())
        {
            mCharacterRepository->exec(QString{"UPDATE %1 SET key_id = NULL, enabled = 0"}.arg(mCharacterRepository->getTableName()));
            emit taskEnded(task, QString{});
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
                            emit taskEnded(charListSubtask, error);
                        }
                        else
                        {
                            for (const auto id : characters)
                                importCharacter(id, charListSubtask, key);
                        }
                    }
                    else
                    {
                        emit taskEnded(charListSubtask, error);
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
                if (error.isEmpty())
                {
                    auto query = mAssetListRepository->prepare(QString{"DELETE FROM %1 WHERE character_id = ?"}
                        .arg(mAssetListRepository->getTableName()));
                    query.bindValue(0, id);

                    Evernus::DatabaseUtils::execQuery(query);

                    const auto it = mCharacterAssets.find(id);
                    if (it != std::end(mCharacterAssets))
                        *it->second = data;

                    mAssetListRepository->store(data);

                    QSettings settings;

                    if (settings.value(Evernus::ImportSettings::autoUpdateAssetValueKey, true).toBool())
                        computeAssetListSellValue(data);

                    saveUpdateTimer(Evernus::TimerType::AssetList, mAssetsUtcUpdateTimes, id);

                    emit assetsChanged();
                }

                emit taskEnded(assetSubtask, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(assetSubtask, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(assetSubtask, tr("Character not found!"));
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

            if (maxId == WalletJournalEntry::invalidId)
                emit taskInfoChanged(task, tr("Fetching wallet journal for character %1 (this may take a while)...").arg(id));

            mAPIManager.fetchWalletJournal(key, id, WalletJournalEntry::invalidId, maxId,
                                           [task, id, this](const auto &data, const auto &error) {
                if (error.isEmpty())
                {
                    std::vector<Evernus::WalletSnapshot> snapshots;
                    snapshots.reserve(data.size());

                    QSet<QDateTime> usedSnapshots;

                    for (auto &entry : data)
                    {
                        const auto timestamp = entry.getTimestamp();

                        if (!usedSnapshots.contains(timestamp))
                        {
                            Evernus::WalletSnapshot snapshot;
                            snapshot.setTimestamp(timestamp);
                            snapshot.setBalance(entry.getBalance());
                            snapshot.setCharacterId(entry.getCharacterId());

                            snapshots.emplace_back(std::move(snapshot));
                            usedSnapshots << timestamp;
                        }
                    }

                    mWalletJournalEntryRepository->batchStore(data, true);
                    mWalletSnapshotRepository->batchStore(snapshots, false);

                    saveUpdateTimer(Evernus::TimerType::WalletJournal, mWalletJournalUtcUpdateTimes, id);

                    emit walletJournalChanged();
                }

                emit taskEnded(task, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshWalletTransactions(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing wallet transactions: " << id;

        const auto task = startTask(tr("Fetching wallet transactions for character %1...").arg(id));
        processEvents();

        try
        {
            const auto key = getCharacterKey(id);
            const auto maxId = mWalletTransactionRepository->getLatestEntryId(id);

            if (maxId == WalletTransaction::invalidId)
                emit taskInfoChanged(task, tr("Fetching wallet transactions for character %1 (this may take a while)...").arg(id));

            mAPIManager.fetchWalletTransactions(key, id, WalletTransaction::invalidId, maxId,
                                                [task, id, this](const auto &data, const auto &error) {
                if (error.isEmpty())
                {
                    mWalletTransactionRepository->batchStore(data, true);
                    saveUpdateTimer(Evernus::TimerType::WalletTransactions, mWalletTransactionsUtcUpdateTimes, id);

                    emit walletTransactionsChanged();
                }

                emit taskEnded(task, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshMarketOrdersFromAPI(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing market orders from API: " << id;

        const auto task = startTask(tr("Fetching market orders for character %1...").arg(id));
        processEvents();

        try
        {
            const auto key = getCharacterKey(id);
            mAPIManager.fetchMarketOrders(key, id, [task, id, this](auto data, const auto &error) {
                if (error.isEmpty())
                {
                    importMarketLogs(id, data);
                    emit marketOrdersChanged();
                }

                emit taskEnded(task, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshMarketOrdersFromLogs(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing market orders from logs: " << id;

        const auto task = startTask(tr("Fetching market orders for character %1...").arg(id));
        processEvents();

        const auto logPath = PathUtils::getMarketLogsPath();
        if (logPath.isEmpty())
        {
            emit taskEnded(task, tr("Cannot determine market logs path!"));
            return;
        }

        const QDir logDir{logPath};
        const auto logs = logDir.entryList(QStringList{"My Orders*.txt"}, QDir::Files | QDir::Readable, QDir::Time);
        if (logs.isEmpty())
        {
            emit taskEnded(task, tr("No market logs found!"));
            return;
        }

        for (const auto &log : logs)
        {
            qDebug() << "Parsing" << log;

            QFile file{logDir.filePath(log)};
            if (!file.open(QIODevice::ReadOnly))
            {
                emit taskEnded(task, tr("Could not open market log file!"));
                return;
            }

            file.readLine();

            auto characterFound = false;

            MarketOrders orders;
            while (!file.atEnd())
            {
                const QString line = file.readLine();
                const auto values = line.split(',');

                if (values.size() >= 22)
                {
                    if (!characterFound)
                    {
                        const auto characterId = values[2].toULongLong();
                        if (characterId != id)
                        {
                            file.close();
                            continue;
                        }

                        characterFound = true;
                    }

                    MarketOrder order{values[0].toULongLong()};
                    order.setCharacterId(id);
                    order.setLocationId(values[6].toULongLong());
                    order.setVolumeEntered(values[11].toUInt());
                    order.setVolumeRemaining(static_cast<uint>(values[12].toDouble()));
                    order.setMinVolume(values[15].toUInt());
                    order.setDelta(order.getVolumeRemaining() - order.getVolumeEntered());
                    order.setState(static_cast<MarketOrder::State>(values[14].toInt()));
                    order.setTypeId(values[1].toUInt());
                    order.setRange(values[8].toShort());
                    order.setDuration(values[17].toShort());
                    order.setEscrow(values[21].toDouble());
                    order.setPrice(values[10].toDouble());
                    order.setType((values[9] == "True") ? (MarketOrder::Type::Buy) : (MarketOrder::Type::Sell));

                    auto issued = QDateTime::fromString(values[13], "yyyy-MM-dd HH:mm:ss.zzz");
                    issued.setTimeSpec(Qt::UTC);

                    order.setIssued(issued);
                    order.setFirstSeen(issued);

                    orders.emplace_back(std::move(order));
                }
            }

            if (characterFound)
            {
                QSettings settings;
                if (settings.value(PathSettings::deleteLogsKey, true).toBool())
                    file.remove();

                importMarketLogs(id, orders);

                emit marketOrdersChanged();
                break;
            }
        }

        emit taskEnded(task, QString{});
    }

    void EvernusApplication::refreshConquerableStations()
    {
        qDebug() << "Refreshing conquerable stations...";

        const auto task = startTask(tr("Fetching conquerable stations..."));
        processEvents();

        mAPIManager.fetchConquerableStationList([task, this](const auto &list, const auto &error) {
            if (error.isEmpty())
            {
                mConquerableStationRepository->exec(QString{"DELETE FROM %1"}.arg(mConquerableStationRepository->getTableName()));
                mConquerableStationRepository->batchStore(list, true);

                emit conquerableStationsChanged();
            }

            emit taskEnded(task, error);
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

    void EvernusApplication::updateAssetsValue(Character::IdType id)
    {
        computeAssetListSellValue(fetchAssetsForCharacter(id));
    }

    void EvernusApplication::resetItemCostCache()
    {
        mItemCostCache.clear();
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
        mUpdateTimerRepository.reset(new UpdateTimerRepository{mMainDb});
        mWalletTransactionRepository.reset(new WalletTransactionRepository{mMainDb});
        mMarketOrderRepository.reset(new MarketOrderRepository{mMainDb});
        mItemCostRepository.reset(new ItemCostRepository{mMainDb});
        mMarketOrderValueSnapshotRepository.reset(new MarketOrderValueSnapshotRepository{mMainDb});
        mEveTypeRepository.reset(new EveTypeRepository{mEveDb});
        mMarketGroupRepository.reset(new MarketGroupRepository{mEveDb});
        mMetaGroupRepository.reset(new MetaGroupRepository{mEveDb});
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
        mUpdateTimerRepository->create(*mCharacterRepository);
        mWalletTransactionRepository->create(*mCharacterRepository);
        mMarketOrderRepository->create(*mCharacterRepository);
        mItemCostRepository->create(*mCharacterRepository);
        mMarketOrderValueSnapshotRepository->create(*mCharacterRepository);
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
            switch (timer.getType()) {
            case TimerType::Character:
                mCharacterUtcCacheTimes[timer.getCharacterId()] = timer.getCacheUntil();
                break;
            case TimerType::AssetList:
                mAssetsUtcCacheTimes[timer.getCharacterId()] = timer.getCacheUntil();
                break;
            case TimerType::WalletJournal:
                mWalletJournalUtcCacheTimes[timer.getCharacterId()] = timer.getCacheUntil();
                break;
            case TimerType::WalletTransactions:
                mWalletTransactionsUtcCacheTimes[timer.getCharacterId()] = timer.getCacheUntil();
                break;
            case TimerType::MarketOrders:
                mMarketOrdersUtcCacheTimes[timer.getCharacterId()] = timer.getCacheUntil();
            }
        }
    }

    void EvernusApplication::deleteOldWalletEntries()
    {
        QSettings settings;

        if (settings.value(WalletSettings::deleteOldJournalKey, true).toBool())
        {
            const auto journalDt = QDateTime::currentDateTimeUtc().addDays(
                 -settings.value(WalletSettings::oldJournalDaysKey, WalletSettings::oldJournalDaysDefault).toInt());
            mWalletJournalEntryRepository->deleteOldEntires(journalDt);
        }

        if (settings.value(WalletSettings::deleteOldTransactionsKey, true).toBool())
        {
            const auto transactionDt = QDateTime::currentDateTimeUtc().addDays(
                 -settings.value(WalletSettings::oldJournalDaysKey, WalletSettings::oldJournalDaysDefault).toInt());
            mWalletTransactionRepository->deleteOldEntires(transactionDt);
        }
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
        mAPIManager.fetchCharacter(key, id, [charSubtask, id, this](auto data, const auto &error) {
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

                Evernus::WalletSnapshot snapshot;
                snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
                snapshot.setBalance(data.getISK());
                snapshot.setCharacterId(data.getId());
                mWalletSnapshotRepository->store(snapshot);

                saveUpdateTimer(Evernus::TimerType::Character, mCharacterUtcUpdateTimes, id);

                QMetaObject::invokeMethod(this, "scheduleCharacterUpdate", Qt::QueuedConnection);
            }

            emit taskEnded(charSubtask, error);
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

    void EvernusApplication::importMarketLogs(Character::IdType id, MarketOrders &orders)
    {
        const auto curStates = mMarketOrderRepository->getOrderStatesAndVolumes(id);

        MarketOrderRepository::OrderIdList idsToArchive;
        for (const auto &cur : curStates)
            idsToArchive.emplace(cur.first);

        mSellOrders.clear();
        mBuyOrders.clear();
        mArchivedOrders.clear();

        const auto addToCache = [id, this](const auto &order) {
            if (order.getType() == Evernus::MarketOrder::Type::Buy)
                mBuyOrders[id].emplace_back(order);
            else
                mSellOrders[id].emplace_back(order);
        };

        for (auto it = std::begin(orders); it != std::end(orders);)
        {
            idsToArchive.erase(it->getId());

            const auto cIt = curStates.find(it->getId());
            if (cIt != std::end(curStates))
            {
                it->setDelta(it->getVolumeRemaining() - cIt->second.mVolumeRemaining);
                it->setFirstSeen(cIt->second.mFirstSeen);

                if (it->getState() != MarketOrder::State::Active)
                {
                    if (cIt->second.mState == MarketOrder::State::Archived)
                    {
                        it = orders.erase(it);
                    }
                    else
                    {
                        if (cIt->second.mState != MarketOrder::State::Active)
                        {
                            it->setState(MarketOrder::State::Archived);
                        }
                        else
                        {
                            it->setLastSeen(std::min(QDateTime::currentDateTimeUtc(), it->getIssued().addDays(it->getDuration())));
                            addToCache(*it);
                        }

                        ++it;
                    }
                }
                else
                {
                    addToCache(*it);
                    ++it;
                }
            }
            else
            {
                it->setDelta(it->getVolumeRemaining() - it->getVolumeEntered());
                addToCache(*it);
                ++it;
            }
        }

        mMarketOrderRepository->archive(idsToArchive);

        MarketOrderValueSnapshot snapshot;
        snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
        snapshot.setCharacterId(id);

        double buy = 0., sell = 0.;
        for (const auto &order : orders)
        {
            if (order.getState() != MarketOrder::State::Active)
                continue;

            if (order.getType() == MarketOrder::Type::Buy)
                buy += order.getPrice() * order.getVolumeRemaining();
            else
                sell += order.getPrice() * order.getVolumeRemaining();
        }

        snapshot.setBuyValue(buy);
        snapshot.setSellValue(sell);

        mMarketOrderValueSnapshotRepository->store(snapshot);
        mMarketOrderRepository->batchStore(orders, true);

        saveUpdateTimer(TimerType::MarketOrders, mMarketOrdersUtcUpdateTimes, id);
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

        emit taskEnded(task, info);
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

    ItemPrice EvernusApplication::getTypeBuyPrice(EveType::IdType id, quint64 stationId, bool dontThrow) const
    {
        const auto key = std::make_pair(id, stationId);
        const auto it = mBuyPrices.find(key);
        if (it != std::end(mBuyPrices))
            return it->second;

        ItemPrice result;

        try
        {
            result = mItemPriceRepository->findBuyByTypeAndLocation(id, stationId);
        }
        catch (const ItemPriceRepository::NotFoundException &)
        {
            if (!dontThrow)
                throw;
        }

        mBuyPrices.emplace(key, result);
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

            AssetValueSnapshot snapshot;
            snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
            snapshot.setBalance(value);
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

    ItemPrice EvernusApplication::saveTypePrice(ItemPrice::Type type,
                                                quint64 stationId,
                                                EveType::IdType typeId,
                                                const QDateTime &priceTime,
                                                double price) const
    {
        ItemPrice item;
        item.setType(type);
        item.setLocationId(stationId);
        item.setTypeId(typeId);
        item.setUpdateTime(priceTime);
        item.setValue(price);

        mItemPriceRepository->store(item);

        return item;
    }

    void EvernusApplication::saveUpdateTimer(TimerType timer, CharacterTimerMap &map, Character::IdType characterId) const
    {
        const auto time = QDateTime::currentDateTimeUtc();

        map[characterId] = time;

        UpdateTimer storedTimer;
        storedTimer.setCharacterId(characterId);
        storedTimer.setType(timer);
        storedTimer.setUpdateTime(time);

        mUpdateTimerRepository->store(storedTimer);
    }

    const EveType &EvernusApplication::getEveType(EveType::IdType id) const
    {
        const auto it = mTypeCache.find(id);
        if (it != std::end(mTypeCache))
            return it->second;

        EveType type;

        try
        {
            type = mEveTypeRepository->find(id);
        }
        catch (const EveTypeRepository::NotFoundException &)
        {
        }

        return mTypeCache.emplace(id, type).first->second;
    }

    const MarketGroup &EvernusApplication::getMarketGroupParent(MarketGroup::IdType id) const
    {
        const auto it = mTypeMarketGroupParentCache.find(id);
        if (it != std::end(mTypeMarketGroupParentCache))
            return it->second;

        MarketGroup result;

        try
        {
            result = mMarketGroupRepository->findParent(id);
        }
        catch (const MarketGroupRepository::NotFoundException &)
        {
        }

        return mTypeMarketGroupParentCache.emplace(id, result).first->second;
    }

    void EvernusApplication::showSplashMessage(const QString &message, QSplashScreen &splash)
    {
        splash.showMessage(message, Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
        processEvents();
    }
}
