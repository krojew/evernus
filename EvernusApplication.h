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
#pragma once

#include <functional>
#include <memory>

#include <boost/functional/hash.hpp>

#include <QApplication>
#include <QSqlDatabase>
#include <QTranslator>

#include "CharacterCorporationCombinedMarketOrderProvider.h"
#include "MarketOrderValueSnapshotRepository.h"
#include "ExternalOrderImporterRegistry.h"
#include "ConquerableStationRepository.h"
#include "AssetValueSnapshotRepository.h"
#include "WalletJournalEntryRepository.h"
#include "WalletTransactionRepository.h"
#include "CachingMarketOrderProvider.h"
#include "LocationBookmarkRepository.h"
#include "WalletSnapshotRepository.h"
#include "ExternalOrderRepository.h"
#include "CachingContractProvider.h"
#include "FavoriteItemRepository.h"
#include "ExternalOrderImporter.h"
#include "MarketGroupRepository.h"
#include "UpdateTimerRepository.h"
#include "OrderScriptRepository.h"
#include "qxthttpsessionmanager.h"
#include "CacheTimerRepository.h"
#include "FilterTextRepository.h"
#include "CharacterRepository.h"
#include "AssetListRepository.h"
#include "MetaGroupRepository.h"
#include "ItemCostRepository.h"
#include "CacheTimerProvider.h"
#include "ContractRepository.h"
#include "EveTypeRepository.h"
#include "RefTypeRepository.h"
#include "CorpKeyRepository.h"
#include "ItemCostProvider.h"
#include "EveDataProvider.h"
#include "ItemRepository.h"
#include "AssetProvider.h"
#include "TaskConstants.h"
#include "KeyRepository.h"
#include "APIManager.h"
#include "qxtsmtp.h"

class QSplashScreen;

namespace Evernus
{
    class Key;

    class EvernusApplication
        : public QApplication
        , public EveDataProvider
        , public ExternalOrderImporterRegistry
        , public AssetProvider
        , public CacheTimerProvider
        , public ItemCostProvider
    {
        Q_OBJECT

    public:
        static const QString versionKey;

        EvernusApplication(int &argc, char *argv[]);
        virtual ~EvernusApplication() = default;

        virtual QString getTypeName(EveType::IdType id) const override;
        virtual QString getTypeMarketGroupParentName(EveType::IdType id) const override;
        virtual QString getTypeMarketGroupName(EveType::IdType id) const override;
        virtual MarketGroup::IdType getTypeMarketGroupParentId(EveType::IdType id) const override;
        virtual const std::unordered_map<EveType::IdType, QString> &getAllTypeNames() const override;
        virtual QString getTypeMetaGroupName(EveType::IdType id) const override;

        virtual double getTypeVolume(EveType::IdType id) const override;
        virtual std::shared_ptr<ExternalOrder> getTypeSellPrice(EveType::IdType id, quint64 stationId) const override;
        virtual std::shared_ptr<ExternalOrder> getTypeBuyPrice(EveType::IdType id, quint64 stationId) const override;

        virtual void updateExternalOrders(const std::vector<ExternalOrder> &orders) override;

        virtual QString getLocationName(quint64 id) const override;
        virtual QString getRegionName(uint id) const override;
        virtual QString getSolarSystemName(uint id) const override;

        virtual QString getRefTypeName(uint id) const override;

        virtual const std::vector<MapLocation> &getRegions() const override;
        virtual const std::vector<MapLocation> &getConstellations(uint regionId) const override;
        virtual const std::vector<MapLocation> &getSolarSystems(uint constellationId) const override;
        virtual const std::vector<Station> &getStations(uint solarSystemId) const override;

        virtual double getSolarSystemSecurityStatus(uint solarSystemId) const override;

        virtual void registerImporter(const std::string &name, std::unique_ptr<ExternalOrderImporter> &&importer) override;

        virtual std::shared_ptr<AssetList> fetchAssetsForCharacter(Character::IdType id) const override;

        virtual QDateTime getLocalCacheTimer(Character::IdType id, TimerType type) const override;
        virtual void setUtcCacheTimer(Character::IdType id, TimerType type, const QDateTime &dt) override;

        virtual QDateTime getLocalUpdateTimer(Character::IdType id, TimerType type) const override;

        virtual std::shared_ptr<ItemCost> fetchForCharacterAndType(Character::IdType characterId, EveType::IdType typeId) const override;
        virtual CostList fetchForCharacter(Character::IdType characterId) const override;
        virtual void setForCharacterAndType(Character::IdType characterId, EveType::IdType typeId, double value) override;

        virtual std::shared_ptr<ItemCost> findItemCost(ItemCost::IdType id) const override;
        virtual void removeItemCost(ItemCost::IdType id) const override;
        virtual void storeItemCost(ItemCost &cost) const override;
        virtual void removeAllItemCosts(Character::IdType characterId) const override;

        const KeyRepository &getKeyRepository() const noexcept;
        const CorpKeyRepository &getCorpKeyRepository() const noexcept;
        const CharacterRepository &getCharacterRepository() const noexcept;
        const WalletSnapshotRepository &getWalletSnapshotRepository() const noexcept;
        const AssetValueSnapshotRepository &getAssetValueSnapshotRepository() const noexcept;
        const WalletJournalEntryRepository &getWalletJournalEntryRepository() const noexcept;
        const WalletTransactionRepository &getWalletTransactionRepository() const noexcept;
        const MarketOrderRepository &getMarketOrderRepository() const noexcept;
        const WalletJournalEntryRepository &getCorpWalletJournalEntryRepository() const noexcept;
        const WalletTransactionRepository &getCorpWalletTransactionRepository() const noexcept;
        const MarketOrderRepository &getCorpMarketOrderRepository() const noexcept;
        const ItemCostRepository &getItemCostRepository() const noexcept;
        const MarketOrderValueSnapshotRepository &getMarketOrderValueSnapshotRepository() const noexcept;
        const FilterTextRepository &getFilterTextRepository() const noexcept;
        const OrderScriptRepository &getOrderScriptRepository() const noexcept;
        const FavoriteItemRepository &getFavoriteItemRepository() const noexcept;
        const LocationBookmarkRepository &getLocationBookmarkRepository() const noexcept;
        const ExternalOrderRepository &getExternalOrderRepository() const noexcept;

        const MarketOrderProvider &getMarketOrderProvider() const noexcept;
        const MarketOrderProvider &getCorpMarketOrderProvider() const noexcept;

        const ContractProvider &getContractProvider() const noexcept;
        const ContractProvider &getCorpContractProvider() const noexcept;

    signals:
        void taskStarted(uint taskId, const QString &description);
        void taskStarted(uint taskId, uint parentTask, const QString &description);
        void taskInfoChanged(uint taskId, const QString &text);
        void taskEnded(uint taskId, const QString &error);

        void apiError(const QString &info);

        void conquerableStationsChanged();
        void charactersChanged();
        void assetsChanged();
        void externalOrdersChanged();
        void externalOrdersChangedWithMarketOrders();
        void walletJournalChanged();
        void walletTransactionsChanged();
        void marketOrdersChanged();
        void contractsChanged();
        void corpWalletJournalChanged();
        void corpWalletTransactionsChanged();
        void corpMarketOrdersChanged();
        void corpContractsChanged();
        void itemCostsChanged() const;

        void openMarginTool();

    public slots:
        void refreshCharacters();
        void refreshCharacter(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshAssets(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshContracts(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshWalletJournal(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshWalletTransactions(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshMarketOrdersFromAPI(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshMarketOrdersFromLogs(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshCorpContracts(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshCorpWalletJournal(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshCorpWalletTransactions(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshCorpMarketOrdersFromAPI(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshCorpMarketOrdersFromLogs(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshConquerableStations();
        void refreshAllExternalOrders();
        void refreshExternalOrdersFromWeb(const ExternalOrderImporter::TypeLocationPairs &target);
        void refreshExternalOrdersFromFile(const ExternalOrderImporter::TypeLocationPairs &target);
        void refreshExternalOrdersFromCache(const ExternalOrderImporter::TypeLocationPairs &target);

        void updateExternalOrdersAndAssetValue(const std::vector<ExternalOrder> &orders);

        void handleNewPreferences();

        void importFromMentat();

    private slots:
        void scheduleCharacterUpdate();
        void updateCharacters();

        void showPriceImportError(const QString &info);

        void emitNewItemCosts();

        void showSmtpError(const QByteArray &message);
        void showMailError(int mailID, int errorCode, const QByteArray &message);

    private:
        typedef std::pair<EveType::IdType, quint64> TypeLocationPair;
        typedef std::pair<Character::IdType, EveType::IdType> CharacterTypePair;
        typedef std::pair<EveType::IdType, uint> TypeRegionPair;

        typedef std::unordered_map<Character::IdType, QDateTime> CharacterTimerMap;

        typedef std::function<WalletTransactionRepository::EntityList (const QDateTime &, const QDateTime &, EveType::IdType)> TransactionFetcher;

        QSqlDatabase mMainDb, mEveDb;

        std::unique_ptr<KeyRepository> mKeyRepository;
        std::unique_ptr<CorpKeyRepository> mCorpKeyRepository;
        std::unique_ptr<CharacterRepository> mCharacterRepository;
        std::unique_ptr<ItemRepository> mItemRepository;
        std::unique_ptr<AssetListRepository> mAssetListRepository;
        std::unique_ptr<ConquerableStationRepository> mConquerableStationRepository;
        std::unique_ptr<WalletSnapshotRepository> mWalletSnapshotRepository;
        std::unique_ptr<ExternalOrderRepository> mExternalOrderRepository;
        std::unique_ptr<AssetValueSnapshotRepository> mAssetValueSnapshotRepository;
        std::unique_ptr<WalletJournalEntryRepository> mWalletJournalEntryRepository, mCorpWalletJournalEntryRepository;
        std::unique_ptr<RefTypeRepository> mRefTypeRepository;
        std::unique_ptr<CacheTimerRepository> mCacheTimerRepository;
        std::unique_ptr<UpdateTimerRepository> mUpdateTimerRepository;
        std::unique_ptr<WalletTransactionRepository> mWalletTransactionRepository, mCorpWalletTransactionRepository;
        std::unique_ptr<MarketOrderRepository> mMarketOrderRepository, mCorpMarketOrderRepository;
        std::unique_ptr<ItemCostRepository> mItemCostRepository;
        std::unique_ptr<MarketOrderValueSnapshotRepository> mMarketOrderValueSnapshotRepository;
        std::unique_ptr<FilterTextRepository> mFilterTextRepository;
        std::unique_ptr<OrderScriptRepository> mOrderScriptRepository;
        std::unique_ptr<FavoriteItemRepository> mFavoriteItemRepository;
        std::unique_ptr<LocationBookmarkRepository> mLocationBookmarkRepository;
        std::unique_ptr<ContractRepository> mContractRepository, mCorpContractRepository;
        std::unique_ptr<EveTypeRepository> mEveTypeRepository;
        std::unique_ptr<MarketGroupRepository> mMarketGroupRepository;
        std::unique_ptr<MetaGroupRepository> mMetaGroupRepository;

        APIManager mAPIManager;

        uint mTaskId = TaskConstants::invalidTask + 1;
        uint mCurrentExternalOrderImportTask = TaskConstants::invalidTask;

        bool mCharacterUpdateScheduled = false;
        bool mItemCostUpdateScheduled = false;

        mutable std::unordered_map<EveType::IdType, EveTypeRepository::EntityPtr> mTypeCache;
        mutable std::unordered_map<EveType::IdType, QString> mTypeNameCache;
        mutable std::unordered_map<EveType::IdType, MarketGroupRepository::EntityPtr> mTypeMarketGroupParentCache;
        mutable std::unordered_map<EveType::IdType, MarketGroupRepository::EntityPtr> mTypeMarketGroupCache;
        mutable std::unordered_map<EveType::IdType, MetaGroupRepository::EntityPtr> mTypeMetaGroupCache;
        mutable std::unordered_map<quint64, QString> mLocationNameCache;
        mutable std::unordered_map<uint, uint> mSolarSystemRegionCache;
        mutable std::unordered_map<quint64, uint> mLocationSolarSystemCache;

        mutable std::unordered_map<TypeLocationPair, ExternalOrderRepository::EntityPtr, boost::hash<TypeLocationPair>>
        mSellPrices;
        mutable std::unordered_map<TypeLocationPair, ExternalOrderRepository::EntityPtr, boost::hash<TypeLocationPair>>
        mBuyPrices;

        std::unordered_map<std::string, ImporterPtr> mExternalOrderImporters;

        std::unordered_map<RefType::IdType, QString> mRefTypeNames;

        mutable std::unordered_map<Character::IdType, AssetListRepository::EntityPtr> mCharacterAssets;

        CharacterTimerMap mCharacterUtcCacheTimes;
        CharacterTimerMap mAssetsUtcCacheTimes;
        CharacterTimerMap mWalletJournalUtcCacheTimes;
        CharacterTimerMap mWalletTransactionsUtcCacheTimes;
        CharacterTimerMap mMarketOrdersUtcCacheTimes;
        CharacterTimerMap mContractsUtcCacheTimes;
        CharacterTimerMap mCorpWalletJournalUtcCacheTimes;
        CharacterTimerMap mCorpWalletTransactionsUtcCacheTimes;
        CharacterTimerMap mCorpMarketOrdersUtcCacheTimes;
        CharacterTimerMap mCorpContractsUtcCacheTimes;

        CharacterTimerMap mCharacterUtcUpdateTimes;
        CharacterTimerMap mAssetsUtcUpdateTimes;
        CharacterTimerMap mWalletJournalUtcUpdateTimes;
        CharacterTimerMap mWalletTransactionsUtcUpdateTimes;
        CharacterTimerMap mMarketOrdersUtcUpdateTimes;
        CharacterTimerMap mContractsUtcUpdateTimes;
        CharacterTimerMap mCorpWalletJournalUtcUpdateTimes;
        CharacterTimerMap mCorpWalletTransactionsUtcUpdateTimes;
        CharacterTimerMap mCorpMarketOrdersUtcUpdateTimes;
        CharacterTimerMap mCorpContractsUtcUpdateTimes;

        std::unique_ptr<CachingMarketOrderProvider> mCharacterOrderProvider, mCorpOrderProvider;
        std::unique_ptr<CharacterCorporationCombinedMarketOrderProvider> mCombinedOrderProvider;

        std::unique_ptr<CachingContractProvider> mCharacterContractProvider, mCorpContractProvider;

        mutable std::unordered_map<CharacterTypePair, ItemCostRepository::EntityPtr, boost::hash<CharacterTypePair>>
        mCharacterItemCostCache;
        mutable std::unordered_map<EveType::IdType, ItemCostRepository::EntityPtr> mTypeItemCostCache;

        std::unordered_map<uint, std::unordered_multimap<uint, uint>> mSystemJumpMap;

        mutable std::unordered_map<TypeRegionPair, ExternalOrderRepository::EntityList, boost::hash<TypeRegionPair>>
        mTypeRegionOrderCache;

        QTranslator mTranslator, mQtTranslator;

        QxtHttpSessionManager mIGBSessionManager, mHttpSessionManager;
        QxtSmtp mSmtp;

        mutable std::vector<MapLocation> mRegionCache;
        mutable std::unordered_map<uint, std::vector<MapLocation>> mConstellationCache, mSolarSystemCache;
        mutable std::unordered_map<uint, std::vector<Station>> mStationCache;

        mutable std::unordered_map<uint, QString> mRegionNameCache;
        mutable std::unordered_map<uint, QString> mSolarSystemNameCache;

        std::unordered_set<MarketOrder::IdType> mPendingAutoCostOrders;

        mutable std::unordered_map<uint, double> mSecurityStatuses;

        void updateTranslator(const QString &lang);

        void createDb();
        void createDbSchema();
        void precacheRefTypes();
        void precacheCacheTimers();
        void precacheUpdateTimers();
        void precacheJumpMap();
        void deleteOldWalletEntries();

        uint startTask(const QString &description);
        uint startTask(uint parentTask, const QString &description);

        void importCharacter(Character::IdType id, uint parentTask, const Key &key);
        void importExternalOrders(const std::string &importerName, const ExternalOrderImporter::TypeLocationPairs &target);
        void importMarketOrdersFromLogs(Character::IdType id, uint task, bool corp);
        void importMarketOrders(Character::IdType id, MarketOrders &orders, bool corp);

        KeyRepository::EntityPtr getCharacterKey(Character::IdType id) const;
        CorpKeyRepository::EntityPtr getCorpKey(Character::IdType id) const;

        void finishExternalOrderImportTask(const QString &info);

        std::shared_ptr<ExternalOrder> getTypeSellPrice(EveType::IdType id, quint64 stationId, bool dontThrow) const;
        void computeAssetListSellValue(const AssetList &list) const;
        double getTotalItemSellValue(const Item &item, quint64 locationId) const;

        void saveUpdateTimer(TimerType timer, CharacterTimerMap &map, Character::IdType characterId) const;

        EveTypeRepository::EntityPtr getEveType(EveType::IdType id) const;
        MarketGroupRepository::EntityPtr getMarketGroupParent(MarketGroup::IdType id) const;
        MarketGroupRepository::EntityPtr getMarketGroup(MarketGroup::IdType id) const;

        uint getSolarSystemRegionId(uint stationId) const;
        uint getStationSolarSystemId(quint64 stationId) const;

        const ExternalOrderRepository::EntityList &getExternalOrders(EveType::IdType typeId, uint regionId) const;

        void computeAutoCosts(Character::IdType characterId,
                              const MarketOrderProvider::OrderList &orders,
                              const TransactionFetcher &transFetcher);

        void setSmtpSettings();

        void clearExternalOrderCaches();

        static void showSplashMessage(const QString &message, QSplashScreen &splash);
    };
}
