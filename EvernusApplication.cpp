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
#include <unordered_set>
#include <stdexcept>
#include <queue>

#include <QSplashScreen>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
#include <QSet>

#include "ExternalOrderImporterNames.h"
#include "LanguageSelectDialog.h"
#include "UpdaterSettings.h"
#include "ImportSettings.h"
#include "WalletSettings.h"
#include "PriceSettings.h"
#include "PathSettings.h"
#include "IGBSettings.h"
#include "IGBService.h"
#include "UISettings.h"
#include "PathUtils.h"
#include "Updater.h"

#include "EvernusApplication.h"

namespace Evernus
{
    const QString EvernusApplication::versionKey = "version";

    EvernusApplication::EvernusApplication(int &argc, char *argv[])
        : QApplication(argc, argv)
        , EveDataProvider()
        , ExternalOrderImporterRegistry()
        , AssetProvider()
        , CacheTimerProvider()
        , MarketOrderProvider()
        , ItemCostProvider()
        , mMainDb(QSqlDatabase::addDatabase("QSQLITE", "main"))
        , mEveDb(QSqlDatabase::addDatabase("QSQLITE", "eve"))
        , mAPIManager(*this)
    {
        QSettings settings;

        auto lang = settings.value(UISettings::languageKey).toString();
        if (lang.isEmpty())
        {
            lang = QLocale{}.name();
            updateTranslator(lang);

            LanguageSelectDialog dlg;
            if (dlg.exec() == QDialog::Accepted)
            {
                lang = dlg.getSelectedLanguage();
                settings.setValue(UISettings::languageKey, lang);
                updateTranslator(lang);
            }
            else
            {
                settings.setValue(UISettings::languageKey, lang);
            }
        }
        else
        {
            updateTranslator(lang);
        }

        QSplashScreen splash{QPixmap{":/images/splash.png"}};
        splash.show();
        showSplashMessage(tr("Loading..."), splash);

        showSplashMessage(tr("Creating databases..."), splash);
        createDb();

        showSplashMessage(tr("Creating schemas..."), splash);
        createDbSchema();

        showSplashMessage(tr("Precaching ref types..."), splash);
        precacheRefTypes();

        showSplashMessage(tr("Precaching timers..."), splash);
        precacheCacheTimers();
        precacheUpdateTimers();

        showSplashMessage(tr("Precaching jump map..."), splash);
        precacheJumpMap();

        showSplashMessage(tr("Clearing old wallet entries..."), splash);
        deleteOldWalletEntries();

        showSplashMessage(tr("Setting up IGB service..."), splash);
        auto service = new IGBService{*this, *this, &mHttpSessionManager, this};
        connect(service, SIGNAL(openMarginTool()), this, SIGNAL(openMarginTool()));

        mHttpSessionManager.setPort(settings.value(IGBSettings::portKey, IGBSettings::portDefault).value<quint16>());
        mHttpSessionManager.setListenInterface(QHostAddress::LocalHost);
        mHttpSessionManager.setStaticContentService(service);
        mHttpSessionManager.setConnector(QxtHttpSessionManager::HttpServer);

        if (settings.value(IGBSettings::enabledKey, true).toBool())
            mHttpSessionManager.start();

        showSplashMessage(tr("Loading..."), splash);

        Updater::getInstance().performVersionMigration();
        settings.setValue(versionKey, applicationVersion());

        connect(&mAPIManager, &APIManager::generalError, this, &EvernusApplication::apiError);

        if (settings.value(UpdaterSettings::autoUpdateKey, true).toBool())
            Updater::getInstance().checkForUpdates(true);
    }

    QString EvernusApplication::getTypeName(EveType::IdType id) const
    {
        return getEveType(id)->getName();
    }

    QString EvernusApplication::getTypeMarketGroupParentName(EveType::IdType id) const
    {
        const auto type = getEveType(id);
        const auto marketGroupId = type->getMarketGroupId();
        return (marketGroupId) ? (getMarketGroupParent(*marketGroupId)->getName()) : (QString{});
    }

    QString EvernusApplication::getTypeMarketGroupName(EveType::IdType id) const
    {
        const auto type = getEveType(id);
        const auto marketGroupId = type->getMarketGroupId();
        return (marketGroupId) ? (getMarketGroup(*marketGroupId)->getName()) : (QString{});
    }

    MarketGroup::IdType EvernusApplication::getTypeMarketGroupParentId(EveType::IdType id) const
    {
        const auto type = getEveType(id);
        const auto marketGroupId = type->getMarketGroupId();
        return (marketGroupId) ? (getMarketGroupParent(*marketGroupId)->getId()) : (MarketGroup::invalidId);
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
            return it->second->getName();

        MetaGroupRepository::EntityPtr result;

        try
        {
            result = mMetaGroupRepository->fetchForType(id);
        }
        catch (const MetaGroupRepository::NotFoundException &)
        {
            result = std::make_shared<MetaGroup>();
        }

        mTypeMetaGroupCache.emplace(id, result);
        return result->getName();
    }

    double EvernusApplication::getTypeVolume(EveType::IdType id) const
    {
        const auto it = mTypeCache.find(id);
        if (it != std::end(mTypeCache))
            return it->second->getVolume();

        EveTypeRepository::EntityPtr result;

        try
        {
            result = mEveTypeRepository->find(id);
        }
        catch (const EveTypeRepository::NotFoundException &)
        {
            result = std::make_shared<EveType>();
        }

        mTypeCache.emplace(id, result);
        return result->getVolume();
    }

    std::shared_ptr<ExternalOrder> EvernusApplication::getTypeSellPrice(EveType::IdType id, quint64 stationId) const
    {
        return getTypeSellPrice(id, stationId, true);
    }

    std::shared_ptr<ExternalOrder> EvernusApplication::getTypeBuyPrice(EveType::IdType id, quint64 stationId) const
    {
        const auto key = std::make_pair(id, stationId);
        const auto it = mBuyPrices.find(key);
        if (it != std::end(mBuyPrices))
            return it->second;

        auto result = std::make_shared<ExternalOrder>();
        mBuyPrices.emplace(key, result);

        const auto solarSystemId = getStationSolarSystemId(stationId);
        if (solarSystemId == 0)
            return result;

        const auto regionId = getSolarSystemRegionId(solarSystemId);
        if (regionId == 0)
            return result;

        const auto jIt = mSystemJumpMap.find(regionId);
        if (jIt == std::end(mSystemJumpMap))
            return result;

        const auto &jumpMap = jIt->second;
        const auto isReachable = [stationId, solarSystemId, &jumpMap, this](const auto &order) {
            const auto range = order->getRange();
            if (range == -1)
                return stationId == order->getLocationId();

            std::unordered_set<uint> visited;
            std::queue<uint> candidates;

            visited.emplace(solarSystemId);
            candidates.emplace(solarSystemId);

            auto depth = 0;

            auto incrementNode = 0u, lastParentNode = solarSystemId;

            while (!candidates.empty())
            {
                const auto current = candidates.front();
                candidates.pop();

                if (current == incrementNode)
                {
                    ++depth;
                    if (depth > range)
                        return false;
                }

                if (order->getSolarSystemId() == current)
                    return true;

                const auto children = jumpMap.equal_range(current);

                if (current == lastParentNode)
                {
                    if (children.first == children.second)
                    {
                        if (!candidates.empty())
                            lastParentNode = candidates.front();
                    }
                    else
                    {
                        lastParentNode = incrementNode = children.first->second;
                    }
                }

                for (auto it = children.first; it != children.second; ++it)
                {
                    if (visited.find(it->second) == std::end(visited))
                    {
                        visited.emplace(it->second);
                        candidates.emplace(it->second);
                    }
                }
            }

            return false;
        };

        const auto &orders = getExternalOrders(id, regionId);
        for (const auto &order : orders)
        {
            if (order->getValue() <= result->getValue() || !isReachable(order))
                continue;

            result = order;
            mBuyPrices[key] = result;
        }

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
            QSqlQuery query{mEveDb};
            query.prepare("SELECT stationName FROM staStations WHERE stationID = ?");
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
                result = station->getName();
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
                result = station->getName();
            }
            catch (const ConquerableStationRepository::NotFoundException &)
            {
            }
        }
        else if (id > 60000000 && id <= 61000000)
        {
            QSqlQuery query{mEveDb};
            query.prepare("SELECT stationName FROM staStations WHERE stationID = ?");
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
                result = station->getName();
            }
            catch (const ConquerableStationRepository::NotFoundException &)
            {
            }
        }
        else
        {
            QSqlQuery query{mEveDb};
            query.prepare("SELECT itemName FROM mapDenormalize WHERE itemID = ?");
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

    void EvernusApplication::registerImporter(const std::string &name, std::unique_ptr<ExternalOrderImporter> &&importer)
    {
        Q_ASSERT(mExternalOrderImporters.find(name) == std::end(mExternalOrderImporters));

        connect(importer.get(), &ExternalOrderImporter::error, this, &EvernusApplication::showPriceImportError);
        connect(importer.get(), &ExternalOrderImporter::externalOrdersChanged, this, &EvernusApplication::updateExternalOrders);
        mExternalOrderImporters.emplace(name, std::move(importer));
    }

    std::shared_ptr<AssetList> EvernusApplication::fetchAssetsForCharacter(Character::IdType id) const
    {
        const auto it = mCharacterAssets.find(id);
        if (it != std::end(mCharacterAssets))
            return it->second;

        AssetListRepository::EntityPtr assets;

        try
        {
            assets = mAssetListRepository->fetchForCharacter(id);
        }
        catch (const AssetListRepository::NotFoundException &)
        {
            assets = std::make_shared<AssetList>();
            assets->setCharacterId(id);
        }

        mCharacterAssets.emplace(id, assets);
        return assets;
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

        if (type != TimerType::Character)
        {
            CacheTimer timer;
            timer.setCharacterId(id);
            timer.setType(type);
            timer.setCacheUntil(dt);

            mCacheTimerRepository->store(timer);
        }
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

    std::vector<std::shared_ptr<MarketOrder>> EvernusApplication::getSellOrders(Character::IdType characterId) const
    {
        auto it = mSellOrders.find(characterId);
        if (it == std::end(mSellOrders))
            it = mSellOrders.emplace(characterId, mMarketOrderRepository->fetchForCharacter(characterId, MarketOrder::Type::Sell)).first;

        return it->second;
    }

    std::vector<std::shared_ptr<MarketOrder>> EvernusApplication::getBuyOrders(Character::IdType characterId) const
    {
        auto it = mBuyOrders.find(characterId);
        if (it == std::end(mBuyOrders))
            it = mBuyOrders.emplace(characterId, mMarketOrderRepository->fetchForCharacter(characterId, MarketOrder::Type::Buy)).first;

        return it->second;
    }

    std::vector<std::shared_ptr<MarketOrder>> EvernusApplication::getArchivedOrders(Character::IdType characterId, const QDateTime &from, const QDateTime &to) const
    {
        auto it = mArchivedOrders.find(characterId);
        if (it == std::end(mArchivedOrders))
            it = mArchivedOrders.emplace(characterId, mMarketOrderRepository->fetchArchivedForCharacter(characterId)).first;

        std::vector<std::shared_ptr<MarketOrder>> result;
        for (const auto &order : it->second)
        {
            const auto lastSeen = order->getLastSeen();

            if (lastSeen >= from && lastSeen <= to)
                result.emplace_back(order);
        }

        return result;
    }

    std::shared_ptr<ItemCost> EvernusApplication::fetchForCharacterAndType(Character::IdType characterId, EveType::IdType typeId) const
    {
        const auto it = mItemCostCache.find(std::make_pair(characterId, typeId));
        if (it != std::end(mItemCostCache))
            return it->second;

        ItemCostRepository::EntityPtr cost;

        try
        {
            cost = mItemCostRepository->fetchForCharacterAndType(characterId, typeId);
        }
        catch (const ItemCostRepository::NotFoundException &)
        {
            cost = std::make_shared<ItemCost>();
        }

        mItemCostCache.emplace(std::make_pair(characterId, typeId), cost);
        return cost;
    }

    ItemCostProvider::CostList EvernusApplication::fetchForCharacter(Character::IdType characterId) const
    {
        return mItemCostRepository->fetchForCharacter(characterId);
    }

    void EvernusApplication::setForCharacterAndType(Character::IdType characterId, EveType::IdType typeId, double value)
    {
        auto cost = std::make_shared<ItemCost>();
        cost->setCharacterId(characterId);
        cost->setTypeId(typeId);
        cost->setCost(value);

        mItemCostRepository->store(*cost);

        mItemCostCache[std::make_pair(characterId, typeId)] = cost;

        emit itemCostsChanged();
    }

    std::shared_ptr<ItemCost> EvernusApplication::findItemCost(ItemCost::IdType id) const
    {
        return mItemCostRepository->find(id);
    }

    void EvernusApplication::removeItemCost(ItemCost::IdType id) const
    {
        mItemCostRepository->remove(id);
        mItemCostCache.clear();

        emit itemCostsChanged();
    }

    void EvernusApplication::storeItemCost(ItemCost &cost) const
    {
        mItemCostRepository->store(cost);
        mItemCostCache[std::make_pair(cost.getCharacterId(), cost.getTypeId())] = std::make_shared<ItemCost>(cost);

        emit itemCostsChanged();
    }

    void EvernusApplication::removeAllItemCosts(Character::IdType characterId) const
    {
        mItemCostRepository->removeForCharacter(characterId);
        mItemCostCache.clear();

        emit itemCostsChanged();
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

    const FilterTextRepository &EvernusApplication::getFilterTextRepository() const noexcept
    {
        return *mFilterTextRepository;
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
                const auto charListSubtask = startTask(task, tr("Fetching characters for key %1...").arg(key->getId()));
                mAPIManager.fetchCharacterList(*key, [key, charListSubtask, this](const auto &characters, const auto &error) {
                    if (error.isEmpty())
                    {
                        try
                        {
                            if (characters.empty())
                                mCharacterRepository->disableByKey(key->getId());
                            else
                                mCharacterRepository->disableByKey(key->getId(), characters);
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
                                importCharacter(id, charListSubtask, *key);
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
            importCharacter(id, parentTask, *getCharacterKey(id));
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
            mAPIManager.fetchAssets(*key, id, [assetSubtask, id, this](auto data, const auto &error) {
                if (error.isEmpty())
                {
                    mAssetListRepository->deleteForCharacter(id);

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

            mAPIManager.fetchWalletJournal(*key, id, WalletJournalEntry::invalidId, maxId,
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

            mAPIManager.fetchWalletTransactions(*key, id, WalletTransaction::invalidId, maxId,
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
            mAPIManager.fetchMarketOrders(*key, id, [task, id, this](auto data, const auto &error) {
                if (error.isEmpty())
                {
                    importMarketOrders(id, data);
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

                importMarketOrders(id, orders);

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

    void EvernusApplication::refreshExternalOrdersFromWeb(const ExternalOrderImporter::TypeLocationPairs &target)
    {
        importExternalOrders(ExternalOrderImporterNames::webImporter, target);
    }

    void EvernusApplication::refreshExternalOrdersFromFile(const ExternalOrderImporter::TypeLocationPairs &target)
    {
        importExternalOrders(ExternalOrderImporterNames::logImporter, target);
    }

    void EvernusApplication::updateAssetsValue(Character::IdType id)
    {
        computeAssetListSellValue(*fetchAssetsForCharacter(id));
    }

    void EvernusApplication::updateExternalOrders(const std::vector<ExternalOrder> &orders)
    {
        try
        {
            const auto ownOrders = mMarketOrderRepository->getActiveIds();

            ExternalOrderImporter::TypeLocationPairs affectedOrders;

            std::vector<std::reference_wrapper<const ExternalOrder>> toStore;
            for (const auto &order : orders)
            {
                if (ownOrders.find(order.getId()) == std::end(ownOrders))
                {
                    toStore.emplace_back(std::cref(order));
                    affectedOrders.emplace(std::make_pair(order.getTypeId(), order.getLocationId()));
                }
            }

            mSellPrices.clear();
            mBuyPrices.clear();
            mTypeRegionOrderCache.clear();

            mExternalOrderRepository->removeObsolete(affectedOrders);
            mExternalOrderRepository->batchStore(toStore, true);

            QSettings settings;
            if (settings.value(ImportSettings::autoUpdateAssetValueKey, true).toBool())
            {
                for (const auto &list : mCharacterAssets)
                    computeAssetListSellValue(*list.second);
            }

            finishExternalOrderImportTask(QString{});
        }
        catch (const std::exception &e)
        {
            finishExternalOrderImportTask(e.what());
        }

        emit externalOrdersChanged();
    }

    void EvernusApplication::handleNewPreferences()
    {
        QSettings settings;
        updateTranslator(settings.value(UISettings::languageKey).toString());

        mHttpSessionManager.shutdown();
        mHttpSessionManager.setPort(settings.value(IGBSettings::portKey, IGBSettings::portDefault).value<quint16>());

        if (settings.value(IGBSettings::enabledKey, true).toBool())
            mHttpSessionManager.start();
    }

    void EvernusApplication::importFromMentat()
    {
        const auto path = QFileDialog::getExistingDirectory(activeWindow(), tr("Select Mentat directory"));
        if (path.isEmpty())
            return;

        auto db = QSqlDatabase::addDatabase("QSQLITE", "mentat");
        db.setDatabaseName(path + "/Storage/EVEMentat.dynamic.database");
        db.setConnectOptions("QSQLITE_OPEN_READONLY");

        if (!db.open())
        {
            QMessageBox::warning(activeWindow(), tr("Error"), tr("Error opening %1").arg(path + "/Storage/EVEMentat.dynamic.database"));
            return;
        }

        const auto task = startTask(tr("Importing order history..."));
        const auto charIds = mCharacterRepository->fetchAllIds();

        auto query = db.exec(
            "SELECT * FROM mentatOrders o INNER JOIN mentatOrdersHistory h ON h.orderID = o.orderID WHERE o.orderState != 0 AND o.isCorp = 0");

        std::vector<MarketOrder> orders;
        if (query.size() > 0)
            orders.reserve(query.size());

        while (query.next())
        {
            const auto charId = query.value("charID").value<Character::IdType>();
            if (charIds.find(charId) == std::end(charIds))
                continue;

            auto issued = query.value("issued").toDateTime();
            issued.setTimeSpec(Qt::UTC);

            auto lastSeen = query.value("referenceTime").toDateTime();
            lastSeen.setTimeSpec(Qt::UTC);

            auto intState = query.value("orderState").toInt();
            if (intState < static_cast<int>(MarketOrder::State::Active) || intState > static_cast<int>(MarketOrder::State::CharacterDeleted))
                intState = static_cast<int>(MarketOrder::State::Fulfilled);

            MarketOrder order{query.value("orderID").value<MarketOrder::IdType>()};
            order.setAccountKey(query.value("accountID").value<short>());
            order.setCharacterId(charId);
            order.setDuration(query.value("duration").value<short>());
            order.setEscrow(query.value("escrow").toDouble() / 100.);
            order.setFirstSeen(issued);
            order.setIssued(issued);
            order.setLastSeen(lastSeen);
            order.setLocationId(query.value("stationID").toULongLong());
            order.setMinVolume(query.value("minVolume").toUInt());
            order.setPrice(query.value("price").toDouble() / 100.);
            order.setRange(query.value("range").value<short>());
            order.setState(static_cast<MarketOrder::State>(intState));
            order.setType((query.value("bid").toBool()) ? (MarketOrder::Type::Buy) : (MarketOrder::Type::Sell));
            order.setTypeId(query.value("typeID").value<EveType::IdType>());
            order.setVolumeEntered(query.value("volEntered").toUInt());
            order.setVolumeRemaining(query.value("volRemaining").toUInt());

            orders.emplace_back(std::move(order));

            emit taskInfoChanged(task, tr("Importing order history: %1 processed").arg(orders.size()));

            processEvents();
        }

        emit taskInfoChanged(task, tr("Importing order history: storing %1 orders (this may take a while)").arg(orders.size()));

        try
        {
            mMarketOrderRepository->batchStore(orders, true);
            emit taskEnded(task, QString{});
        }
        catch (const std::exception &e)
        {
            emit taskEnded(task, e.what());
        }

        mArchivedOrders.clear();
        emit marketOrdersChanged();
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
        Q_ASSERT(mCurrentExternalOrderImportTask != TaskConstants::invalidTask);
        finishExternalOrderImportTask(info);
    }

    void EvernusApplication::updateTranslator(const QString &lang)
    {
        qDebug() << "Switching language to" << lang;

        const QLocale locale{lang};
        QLocale::setDefault(locale);

        removeTranslator(&mTranslator);
        removeTranslator(&mQtTranslator);

        mTranslator.load(locale, "lang", "_", applicationDirPath() + UISettings::translationPath);
        mQtTranslator.load(locale, "qt", "_", applicationDirPath() + UISettings::translationPath);

        installTranslator(&mQtTranslator);
        installTranslator(&mTranslator);
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
        mExternalOrderRepository.reset(new ExternalOrderRepository{mMainDb});
        mAssetValueSnapshotRepository.reset(new AssetValueSnapshotRepository{mMainDb});
        mWalletJournalEntryRepository.reset(new WalletJournalEntryRepository{mMainDb});
        mRefTypeRepository.reset(new RefTypeRepository{mMainDb});
        mCacheTimerRepository.reset(new CacheTimerRepository{mMainDb});
        mUpdateTimerRepository.reset(new UpdateTimerRepository{mMainDb});
        mWalletTransactionRepository.reset(new WalletTransactionRepository{mMainDb});
        mMarketOrderRepository.reset(new MarketOrderRepository{mMainDb});
        mItemCostRepository.reset(new ItemCostRepository{mMainDb});
        mMarketOrderValueSnapshotRepository.reset(new MarketOrderValueSnapshotRepository{mMainDb});
        mFilterTextRepository.reset(new FilterTextRepository{mMainDb});
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
        mExternalOrderRepository->create();
        mCacheTimerRepository->create(*mCharacterRepository);
        mUpdateTimerRepository->create(*mCharacterRepository);
        mWalletTransactionRepository->create(*mCharacterRepository);
        mMarketOrderRepository->create(*mCharacterRepository);
        mItemCostRepository->create(*mCharacterRepository);
        mMarketOrderValueSnapshotRepository->create(*mCharacterRepository);
        mFilterTextRepository->create();
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
                    for (const auto &ref : refs)
                        mRefTypeNames.emplace(ref.getId(), std::move(ref).getName());
                }
                else
                {
                    qDebug() << error;
                }
            });
        }
        else
        {
            for (const auto &ref : refs)
                mRefTypeNames.emplace(ref->getId(), std::move(*ref).getName());
        }
    }

    void EvernusApplication::precacheCacheTimers()
    {
        const auto timers = mCacheTimerRepository->fetchAll();
        for (const auto &timer : timers)
        {
            switch (timer->getType()) {
            case TimerType::Character:
                mCharacterUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::AssetList:
                mAssetsUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::WalletJournal:
                mWalletJournalUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::WalletTransactions:
                mWalletTransactionsUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::MarketOrders:
                mMarketOrdersUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
            }
        }
    }

    void EvernusApplication::precacheUpdateTimers()
    {
        const auto timers = mUpdateTimerRepository->fetchAll();
        for (const auto &timer : timers)
        {
            switch (timer->getType()) {
            case TimerType::Character:
                mCharacterUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::AssetList:
                mAssetsUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::WalletJournal:
                mWalletJournalUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::WalletTransactions:
                mWalletTransactionsUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::MarketOrders:
                mMarketOrdersUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
            }
        }
    }

    void EvernusApplication::precacheJumpMap()
    {
        auto query = mEveDb.exec("SELECT fromRegionID, fromSolarSystemID, toSolarSystemID FROM mapSolarSystemJumps WHERE fromRegionID = toRegionID");
        while (query.next())
            mSystemJumpMap[query.value(0).toUInt()].emplace(query.value(1).toUInt(), query.value(2).toUInt());
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
                        data.setOrderAmountSkills(prevData->getOrderAmountSkills());
                        data.setTradeRangeSkills(prevData->getTradeRangeSkills());
                        data.setFeeSkills(prevData->getFeeSkills());
                        data.setContractSkills(prevData->getContractSkills());
                    }

                    data.setEnabled(prevData->isEnabled());
                }
                catch (const Evernus::CharacterRepository::NotFoundException &)
                {
                }

                mMainDb.exec("PRAGMA foreign_keys = OFF;");
                mCharacterRepository->store(data);
                mMainDb.exec("PRAGMA foreign_keys = ON;");

                const auto cacheTimer = mCharacterUtcCacheTimes[id];
                if (cacheTimer.isValid())
                {
                    Evernus::CacheTimer timer;
                    timer.setCharacterId(id);
                    timer.setType(Evernus::TimerType::Character);
                    timer.setCacheUntil(cacheTimer);

                    mCacheTimerRepository->store(timer);
                }

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

    void EvernusApplication::importExternalOrders(const std::string &importerName, const ExternalOrderImporter::TypeLocationPairs &target)
    {
        if (mCurrentExternalOrderImportTask != TaskConstants::invalidTask)
            return;

        qDebug() << "Refreshing item prices using importer:" << importerName.c_str();

        const auto it = mExternalOrderImporters.find(importerName);
        Q_ASSERT(it != std::end(mExternalOrderImporters));

        mCurrentExternalOrderImportTask = startTask(tr("Importing item prices..."));
        processEvents();

        it->second->fetchExternalOrders(target);
    }

    void EvernusApplication::importMarketOrders(Character::IdType id, MarketOrders &orders)
    {
        auto curStates = mMarketOrderRepository->getOrderStates(id);

        mSellOrders.erase(id);
        mBuyOrders.erase(id);
        mArchivedOrders.erase(id);

        QSettings settings;
        const auto autoSetCosts = settings.value(PriceSettings::autoAddCustomItemCostKey, true).toBool();

        struct ItemCostData
        {
            uint mQuantity;
            double mPrice;
        };

        std::unordered_map<EveType::IdType, ItemCostData> newItemCosts;

        for (auto &order : orders)
        {
            const auto cIt = curStates.find(order.getId());
            if (cIt != std::end(curStates))
            {
                order.setDelta(order.getVolumeRemaining() - cIt->second.mVolumeRemaining);
                order.setFirstSeen(cIt->second.mFirstSeen);

                if (order.getState() != MarketOrder::State::Active)
                {
                    if (!cIt->second.mLastSeen.isNull())
                        order.setLastSeen(cIt->second.mLastSeen);
                    else
                        order.setLastSeen(std::min(QDateTime::currentDateTimeUtc(), order.getIssued().addDays(order.getDuration())));
                }

                curStates.erase(cIt);
            }
            else
            {
                order.setDelta(order.getVolumeRemaining() - order.getVolumeEntered());
            }

            if (autoSetCosts && order.getType() == MarketOrder::Type::Buy && order.getDelta() != 0 && order.getState() == MarketOrder::State::Fulfilled)
            {
                const auto lastSeen = std::min(QDateTime::currentDateTimeUtc(), order.getIssued().addDays(order.getDuration()));
                const auto transactions
                    = mWalletTransactionRepository->fetchForCharacterInRange(id,
                                                                             order.getFirstSeen(),
                                                                             lastSeen,
                                                                             WalletTransactionRepository::EntryType::Buy,
                                                                             order.getTypeId());

                auto &cost = newItemCosts[order.getTypeId()];
                for (const auto &transaction : transactions)
                {
                    cost.mQuantity += transaction->getQuantity();
                    cost.mPrice += transaction->getQuantity() * transaction->getPrice();
                }
            }
        }

        if (autoSetCosts)
        {
            for (const auto &cost : newItemCosts)
                setForCharacterAndType(id, cost.first, cost.second.mPrice / cost.second.mQuantity);
        }

        std::vector<MarketOrder::IdType> toArchive;
        toArchive.reserve(curStates.size());

        for (const auto &order : curStates)
        {
            if (order.second.mLastSeen.isNull())
                toArchive.emplace_back(order.first);
        }

        if (!toArchive.empty())
            mMarketOrderRepository->archive(toArchive);

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

    KeyRepository::EntityPtr EvernusApplication::getCharacterKey(Character::IdType id) const
    {
        try
        {
            const auto character = mCharacterRepository->find(id);
            const auto keyId = character->getKeyId();

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

    void EvernusApplication::finishExternalOrderImportTask(const QString &info)
    {
        const auto task = mCurrentExternalOrderImportTask;
        mCurrentExternalOrderImportTask = TaskConstants::invalidTask;

        emit taskEnded(task, info);
    }

    std::shared_ptr<ExternalOrder> EvernusApplication::getTypeSellPrice(EveType::IdType id, quint64 stationId, bool dontThrow) const
    {
        const auto key = std::make_pair(id, stationId);
        const auto it = mSellPrices.find(key);
        if (it != std::end(mSellPrices))
            return it->second;

        std::shared_ptr<ExternalOrder> result;

        try
        {
            result = mExternalOrderRepository->findSellByTypeAndLocation(id, stationId);
        }
        catch (const ExternalOrderRepository::NotFoundException &)
        {
            if (!dontThrow)
                throw;

            result = std::make_shared<ExternalOrder>();
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

            AssetValueSnapshot snapshot;
            snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
            snapshot.setBalance(value);
            snapshot.setCharacterId(list.getCharacterId());

            mAssetValueSnapshotRepository->store(snapshot);
        }
        catch (const ExternalOrderRepository::NotFoundException &)
        {
        }
    }

    double EvernusApplication::getTotalItemSellValue(const Item &item, quint64 locationId) const
    {
        auto price = getTypeSellPrice(item.getTypeId(), locationId, false)->getValue() * item.getQuantity();
        for (const auto &child : item)
            price += getTotalItemSellValue(*child, locationId);

        return price;
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

    EveTypeRepository::EntityPtr EvernusApplication::getEveType(EveType::IdType id) const
    {
        const auto it = mTypeCache.find(id);
        if (it != std::end(mTypeCache))
            return it->second;

        EveTypeRepository::EntityPtr type;

        try
        {
            type = mEveTypeRepository->find(id);
        }
        catch (const EveTypeRepository::NotFoundException &)
        {
            type = std::make_shared<EveType>();
        }

        return mTypeCache.emplace(id, type).first->second;
    }

    MarketGroupRepository::EntityPtr EvernusApplication::getMarketGroupParent(MarketGroup::IdType id) const
    {
        const auto it = mTypeMarketGroupParentCache.find(id);
        if (it != std::end(mTypeMarketGroupParentCache))
            return it->second;

        MarketGroupRepository::EntityPtr result;

        try
        {
            result = mMarketGroupRepository->findParent(id);
        }
        catch (const MarketGroupRepository::NotFoundException &)
        {
            result = std::make_shared<MarketGroup>();
        }

        return mTypeMarketGroupParentCache.emplace(id, result).first->second;
    }

    MarketGroupRepository::EntityPtr EvernusApplication::getMarketGroup(MarketGroup::IdType id) const
    {
        const auto it = mTypeMarketGroupCache.find(id);
        if (it != std::end(mTypeMarketGroupCache))
            return it->second;

        MarketGroupRepository::EntityPtr result;

        try
        {
            result = mMarketGroupRepository->find(id);
        }
        catch (const MarketGroupRepository::NotFoundException &)
        {
            result = std::make_shared<MarketGroup>();
        }

        return mTypeMarketGroupCache.emplace(id, result).first->second;
    }

    uint EvernusApplication::getSolarSystemRegionId(uint systemId) const
    {
        const auto it = mSolarSystemRegionCache.find(systemId);
        if (it != std::end(mSolarSystemRegionCache))
            return it->second;

        QSqlQuery query{mEveDb};
        query.prepare("SELECT regionID FROM mapSolarSystems WHERE solarSystemID = ?");
        query.bindValue(0, systemId);

        DatabaseUtils::execQuery(query);
        query.next();

        const auto regionId = query.value(0).toUInt();

        mSolarSystemRegionCache.emplace(systemId, regionId);
        return regionId;
    }

    uint EvernusApplication::getStationSolarSystemId(quint64 stationId) const
    {
        const auto it = mLocationSolarSystemCache.find(stationId);
        if (it != std::end(mLocationSolarSystemCache))
            return it->second;

        uint systemId = 0;
        if (stationId >= 66000000 && stationId <= 66014933)
        {
            QSqlQuery query{mEveDb};
            query.prepare("SELECT solarSystemID FROM staStations WHERE stationID = ?");
            query.bindValue(0, stationId - 6000001);

            DatabaseUtils::execQuery(query);
            query.next();

            systemId = query.value(0).toUInt();
        }
        else if (stationId >= 66014934 && stationId <= 67999999)
        {
            try
            {
                auto station = mConquerableStationRepository->find(stationId - 6000000);
                systemId = station->getSolarSystemId();
            }
            catch (const ConquerableStationRepository::NotFoundException &)
            {
            }
        }
        else if (stationId >= 60014861 && stationId <= 60014928)
        {
            try
            {
                auto station = mConquerableStationRepository->find(stationId);
                systemId = station->getSolarSystemId();
            }
            catch (const ConquerableStationRepository::NotFoundException &)
            {
            }
        }
        else if (stationId > 60000000 && stationId <= 61000000)
        {
            QSqlQuery query{mEveDb};
            query.prepare("SELECT solarSystemID FROM staStations WHERE stationID = ?");
            query.bindValue(0, stationId);

            DatabaseUtils::execQuery(query);
            query.next();

            systemId = query.value(0).toUInt();
        }
        else if (stationId > 61000000)
        {
            try
            {
                auto station = mConquerableStationRepository->find(stationId);
                systemId = station->getSolarSystemId();
            }
            catch (const ConquerableStationRepository::NotFoundException &)
            {
            }
        }
        else
        {
            QSqlQuery query{mEveDb};
            query.prepare("SELECT solarSystemID FROM mapDenormalize WHERE itemID = ?");
            query.bindValue(0, stationId);

            DatabaseUtils::execQuery(query);
            query.next();

            systemId = query.value(0).toUInt();
        }

        mLocationSolarSystemCache.emplace(stationId, systemId);
        return systemId;
    }

    const ExternalOrderRepository::EntityList &EvernusApplication::getExternalOrders(EveType::IdType typeId, uint regionId) const
    {
        const auto key = std::make_pair(typeId, regionId);
        const auto it = mTypeRegionOrderCache.find(key);
        if (it != std::end(mTypeRegionOrderCache))
            return it->second;

        return mTypeRegionOrderCache.emplace(key, mExternalOrderRepository->findBuyByTypeAndRegion(typeId, regionId)).first->second;
    }

    void EvernusApplication::showSplashMessage(const QString &message, QSplashScreen &splash)
    {
        splash.showMessage(message, Qt::AlignBottom | Qt::AlignRight, Qt::white);
        processEvents();
    }
}
