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

#include <QApplication>
#include <QSqlDatabase>
#include <QTranslator>

#include "CharacterCorporationCombinedMarketOrderProvider.h"
#include "CorpMarketOrderValueSnapshotRepository.h"
#include "MarketOrderValueSnapshotRepository.h"
#include "ExternalOrderImporterRegistry.h"
#include "ConquerableStationRepository.h"
#include "AssetValueSnapshotRepository.h"
#include "WalletJournalEntryRepository.h"
#include "CorpWalletSnapshotRepository.h"
#include "WalletTransactionRepository.h"
#include "CachingMarketOrderProvider.h"
#include "LocationBookmarkRepository.h"
#include "WalletSnapshotRepository.h"
#include "ExternalOrderRepository.h"
#include "CachingContractProvider.h"
#include "FavoriteItemRepository.h"
#include "CachingEveDataProvider.h"
#include "ContractItemRepository.h"
#include "ExternalOrderImporter.h"
#include "MarketGroupRepository.h"
#include "UpdateTimerRepository.h"
#include "OrderScriptRepository.h"
#include "qxthttpsessionmanager.h"
#include "CacheTimerRepository.h"
#include "FilterTextRepository.h"
#include "CharacterRepository.h"
#include "LMeveTaskRepository.h"
#include "AssetListRepository.h"
#include "MetaGroupRepository.h"
#include "ItemCostRepository.h"
#include "CacheTimerProvider.h"
#include "RepositoryProvider.h"
#include "ContractRepository.h"
#include "EveTypeRepository.h"
#include "RefTypeRepository.h"
#include "CorpKeyRepository.h"
#include "LMeveDataProvider.h"
#include "ItemCostProvider.h"
#include "LMeveAPIManager.h"
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
        , public ExternalOrderImporterRegistry
        , public AssetProvider
        , public CacheTimerProvider
        , public ItemCostProvider
        , public RepositoryProvider
        , public LMeveDataProvider
    {
        Q_OBJECT

    public:
        static const QString versionKey;

        EvernusApplication(int &argc, char *argv[]);
        virtual ~EvernusApplication() = default;

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

        virtual const KeyRepository &getKeyRepository() const noexcept override;
        virtual const CorpKeyRepository &getCorpKeyRepository() const noexcept override;
        virtual const CharacterRepository &getCharacterRepository() const noexcept override;
        virtual const WalletSnapshotRepository &getWalletSnapshotRepository() const noexcept override;
        virtual const CorpWalletSnapshotRepository &getCorpWalletSnapshotRepository() const noexcept override;
        virtual const AssetValueSnapshotRepository &getAssetValueSnapshotRepository() const noexcept override;
        virtual const WalletJournalEntryRepository &getWalletJournalEntryRepository() const noexcept override;
        virtual const WalletTransactionRepository &getWalletTransactionRepository() const noexcept override;
        virtual const MarketOrderRepository &getMarketOrderRepository() const noexcept override;
        virtual const WalletJournalEntryRepository &getCorpWalletJournalEntryRepository() const noexcept override;
        virtual const WalletTransactionRepository &getCorpWalletTransactionRepository() const noexcept override;
        virtual const MarketOrderRepository &getCorpMarketOrderRepository() const noexcept override;
        virtual const ItemCostRepository &getItemCostRepository() const noexcept override;
        virtual const MarketOrderValueSnapshotRepository &getMarketOrderValueSnapshotRepository() const noexcept override;
        virtual const CorpMarketOrderValueSnapshotRepository &getCorpMarketOrderValueSnapshotRepository() const noexcept override;
        virtual const FilterTextRepository &getFilterTextRepository() const noexcept override;
        virtual const OrderScriptRepository &getOrderScriptRepository() const noexcept override;
        virtual const FavoriteItemRepository &getFavoriteItemRepository() const noexcept override;
        virtual const LocationBookmarkRepository &getLocationBookmarkRepository() const noexcept override;
        virtual const ExternalOrderRepository &getExternalOrderRepository() const noexcept override;

        virtual std::vector<std::shared_ptr<LMeveTask>> getTasks(Character::IdType characterId) const override;

        MarketOrderProvider &getMarketOrderProvider() const noexcept;
        MarketOrderProvider &getCorpMarketOrderProvider() const noexcept;

        const ContractProvider &getContractProvider() const noexcept;
        const ContractProvider &getCorpContractProvider() const noexcept;

        EveDataProvider &getDataProvider() noexcept;

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
        void itemVolumeChanged();
        void lMeveTasksChanged();

        void openMarginTool();

        void showInEve(EveType::IdType id);

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

        void syncLMeve(Character::IdType id);

    private slots:
        void scheduleCharacterUpdate();
        void updateCharacters();

        void scheduleMarketOrderChange();
        void updateMarketOrders();

        void scheduleCorpMarketOrderChange();
        void updateCorpMarketOrders();

        void showPriceImportError(const QString &info);

        void emitNewItemCosts();

        void showSmtpError(const QByteArray &message);
        void showMailError(int mailID, int errorCode, const QByteArray &message);

    private:
        typedef std::pair<Character::IdType, EveType::IdType> CharacterTypePair;

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
        std::unique_ptr<CorpWalletSnapshotRepository> mCorpWalletSnapshotRepository;
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
        std::unique_ptr<CorpMarketOrderValueSnapshotRepository> mCorpMarketOrderValueSnapshotRepository;
        std::unique_ptr<FilterTextRepository> mFilterTextRepository;
        std::unique_ptr<OrderScriptRepository> mOrderScriptRepository;
        std::unique_ptr<FavoriteItemRepository> mFavoriteItemRepository;
        std::unique_ptr<LocationBookmarkRepository> mLocationBookmarkRepository;
        std::unique_ptr<ContractItemRepository> mContractItemRepository, mCorpContractItemRepository;
        std::unique_ptr<ContractRepository> mContractRepository, mCorpContractRepository;
        std::unique_ptr<EveTypeRepository> mEveTypeRepository;
        std::unique_ptr<LMeveTaskRepository> mLMeveTaskRepository;
        std::unique_ptr<MarketGroupRepository> mMarketGroupRepository;
        std::unique_ptr<MetaGroupRepository> mMetaGroupRepository;

        APIManager mAPIManager;
        LMeveAPIManager mLMeveAPIManager;

        uint mTaskId = TaskConstants::invalidTask + 1;
        uint mCurrentExternalOrderImportTask = TaskConstants::invalidTask;

        bool mCharacterUpdateScheduled = false;
        bool mItemCostUpdateScheduled = false;
        bool mMarketOrderUpdateScheduled = false;
        bool mCorpMarketOrderUpdateScheduled = false;

        std::unordered_map<std::string, ImporterPtr> mExternalOrderImporters;

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
        CharacterTimerMap mLMeveUtcCacheTimes;

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

        QTranslator mTranslator, mQtTranslator;

        QxtHttpSessionManager mIGBSessionManager, mHttpSessionManager;
        QxtSmtp mSmtp;

        std::unordered_set<MarketOrder::IdType> mPendingAutoCostOrders;

        std::unique_ptr<CachingEveDataProvider> mDataProvider;

        size_t mPendingContractItemRequests = 0;

        mutable std::unordered_map<Character::IdType, LMeveTaskRepository::EntityList> mLMeveTaskCache;

        void updateTranslator(const QString &lang);

        void createDb();
        void createDbSchema();
        void precacheCacheTimers();
        void precacheUpdateTimers();
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

        void computeAssetListSellValue(const AssetList &list) const;
        double getTotalItemSellValue(const Item &item, quint64 locationId) const;

        void saveUpdateTimer(TimerType timer, CharacterTimerMap &map, Character::IdType characterId) const;

        void computeAutoCosts(Character::IdType characterId,
                              const MarketOrderProvider::OrderList &orders,
                              const TransactionFetcher &transFetcher);

        void setSmtpSettings();

        bool shouldImport(Character::IdType id, TimerType type) const;
        bool checkImportAndEndTask(Character::IdType id, TimerType type, uint task);

        template<void (EvernusApplication::* Signal)(), class Key>
        void handleIncomingContracts(const Key &key,
                                     const Contracts &data,
                                     Character::IdType id,
                                     const ContractItemRepository &itemRepo,
                                     uint task);
        template<void (EvernusApplication::* Signal)(), class Key>
        void doRefreshMarketOrdersFromAPI(const Key &key, Character::IdType id, uint task);

        template<class T, class Data>
        void asyncBatchStore(const T &repo, const Data &data, bool hasId);

        template<class Func, class... Args>
        void asyncExecute(Func &&func, Args && ...args);

        static void showSplashMessage(const QString &message, QSplashScreen &splash);
    };
}
