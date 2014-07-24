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

#include <memory>

#include <boost/functional/hash.hpp>

#include <QApplication>
#include <QSqlDatabase>

#include "MarketOrderValueSnapshotRepository.h"
#include "ConquerableStationRepository.h"
#include "AssetValueSnapshotRepository.h"
#include "WalletJournalEntryRepository.h"
#include "WalletTransactionRepository.h"
#include "ItemPriceImporterRegistry.h"
#include "WalletSnapshotRepository.h"
#include "MarketOrderRepository.h"
#include "MarketGroupRepository.h"
#include "UpdateTimerRepository.h"
#include "CacheTimerRepository.h"
#include "CharacterRepository.h"
#include "AssetListRepository.h"
#include "MetaGroupRepository.h"
#include "ItemPriceRepository.h"
#include "MarketOrderProvider.h"
#include "ItemCostRepository.h"
#include "CacheTimerProvider.h"
#include "ItemPriceImporter.h"
#include "EveTypeRepository.h"
#include "RefTypeRepository.h"
#include "ItemCostProvider.h"
#include "EveDataProvider.h"
#include "ItemRepository.h"
#include "AssetProvider.h"
#include "TaskConstants.h"
#include "KeyRepository.h"
#include "APIManager.h"

class QSplashScreen;

namespace Evernus
{
    class Key;

    class EvernusApplication
        : public QApplication
        , public EveDataProvider
        , public ItemPriceImporterRegistry
        , public AssetProvider
        , public CacheTimerProvider
        , public MarketOrderProvider
        , public ItemCostProvider
    {
        Q_OBJECT

    public:
        EvernusApplication(int &argc, char *argv[]);
        virtual ~EvernusApplication() = default;

        virtual QString getTypeName(EveType::IdType id) const override;
        virtual QString getTypeMarketGroupParentName(EveType::IdType id) const override;
        virtual MarketGroup::IdType getTypeMarketGroupParentId(EveType::IdType id) const override;
        virtual const std::unordered_map<EveType::IdType, QString> &getAllTypeNames() const override;
        virtual QString getTypeMetaGroupName(EveType::IdType id) const override;

        virtual double getTypeVolume(EveType::IdType id) const override;
        virtual ItemPrice getTypeSellPrice(EveType::IdType id, quint64 stationId) const override;
        virtual ItemPrice getTypeBuyPrice(EveType::IdType id, quint64 stationId) const override;

        virtual void setTypeSellPrice(quint64 stationId,
                                      EveType::IdType typeId,
                                      const QDateTime &priceTime,
                                      double price) const override;
        virtual void setTypeBuyPrice(quint64 stationId,
                                     EveType::IdType typeId,
                                     const QDateTime &priceTime,
                                     double price) const override;

        virtual QString getLocationName(quint64 id) const override;

        virtual QString getRefTypeName(uint id) const override;

        virtual void registerImporter(const std::string &name, std::unique_ptr<ItemPriceImporter> &&importer) override;

        virtual const AssetList &fetchAssetsForCharacter(Character::IdType id) const override;

        virtual QDateTime getLocalCacheTimer(Character::IdType id, TimerType type) const override;
        virtual void setUtcCacheTimer(Character::IdType id, TimerType type, const QDateTime &dt) override;

        virtual QDateTime getLocalUpdateTimer(Character::IdType id, TimerType type) const override;

        virtual std::vector<MarketOrder> getSellOrders(Character::IdType characterId) const override;
        virtual std::vector<MarketOrder> getBuyOrders(Character::IdType characterId) const override;
        virtual std::vector<MarketOrder> getArchivedOrders(Character::IdType characterId) const override;

        virtual ItemCost fetchForCharacterAndType(Character::IdType characterId, EveType::IdType typeId) const override;

        const KeyRepository &getKeyRepository() const noexcept;
        const CharacterRepository &getCharacterRepository() const noexcept;
        const WalletSnapshotRepository &getWalletSnapshotRepository() const noexcept;
        const AssetValueSnapshotRepository &getAssetValueSnapshotRepository() const noexcept;
        const WalletJournalEntryRepository &getWalletJournalEntryRepository() const noexcept;
        const WalletTransactionRepository &getWalletTransactionRepository() const noexcept;
        const MarketOrderRepository &getMarketOrderRepository() const noexcept;
        const ItemCostRepository &getItemCostRepository() const noexcept;
        const MarketOrderValueSnapshotRepository &getMarketOrderValueSnapshotRepository() const noexcept;

    signals:
        void taskStarted(uint taskId, const QString &description);
        void taskStarted(uint taskId, uint parentTask, const QString &description);
        void taskInfoChanged(uint taskId, const QString &text);
        void taskEnded(uint taskId, const QString &error);

        void apiError(const QString &info);

        void conquerableStationsChanged();
        void charactersChanged();
        void assetsChanged();
        void itemPricesChanged();
        void walletJournalChanged();
        void walletTransactionsChanged();
        void marketOrdersChanged();

    public slots:
        void refreshCharacters();
        void refreshCharacter(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshAssets(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshWalletJournal(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshWalletTransactions(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshMarketOrdersFromAPI(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshMarketOrdersFromLogs(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshConquerableStations();
        void refreshItemPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &target);
        void refreshItemPricesFromFile(const ItemPriceImporter::TypeLocationPairs &target);

        void updateAssetsValue(Character::IdType id);

        void resetItemCostCache();

    private slots:
        void scheduleCharacterUpdate();
        void updateCharacters();

        void showPriceImportError(const QString &info);
        void updateItemPrices(const std::vector<ItemPrice> &prices);

    private:
        typedef std::pair<EveType::IdType, quint64> TypeLocationPair;
        typedef std::pair<Character::IdType, EveType::IdType> CharacterTypePair;

        typedef std::unordered_map<Character::IdType, QDateTime> CharacterTimerMap;
        typedef std::unordered_map<Character::IdType, std::vector<MarketOrder>> MarketOrderMap;

        static const QString versionKey;

        QSqlDatabase mMainDb, mEveDb;

        std::unique_ptr<KeyRepository> mKeyRepository;
        std::unique_ptr<CharacterRepository> mCharacterRepository;
        std::unique_ptr<ItemRepository> mItemRepository;
        std::unique_ptr<AssetListRepository> mAssetListRepository;
        std::unique_ptr<ConquerableStationRepository> mConquerableStationRepository;
        std::unique_ptr<WalletSnapshotRepository> mWalletSnapshotRepository;
        std::unique_ptr<ItemPriceRepository> mItemPriceRepository;
        std::unique_ptr<AssetValueSnapshotRepository> mAssetValueSnapshotRepository;
        std::unique_ptr<WalletJournalEntryRepository> mWalletJournalEntryRepository;
        std::unique_ptr<RefTypeRepository> mRefTypeRepository;
        std::unique_ptr<CacheTimerRepository> mCacheTimerRepository;
        std::unique_ptr<UpdateTimerRepository> mUpdateTimerRepository;
        std::unique_ptr<WalletTransactionRepository> mWalletTransactionRepository;
        std::unique_ptr<MarketOrderRepository> mMarketOrderRepository;
        std::unique_ptr<ItemCostRepository> mItemCostRepository;
        std::unique_ptr<MarketOrderValueSnapshotRepository> mMarketOrderValueSnapshotRepository;
        std::unique_ptr<EveTypeRepository> mEveTypeRepository;
        std::unique_ptr<MarketGroupRepository> mMarketGroupRepository;
        std::unique_ptr<MetaGroupRepository> mMetaGroupRepository;

        APIManager mAPIManager;

        uint mTaskId = TaskConstants::invalidTask + 1;
        uint mCurrentItemPriceImportTask = TaskConstants::invalidTask;

        bool mCharacterUpdateScheduled = false;

        mutable std::unordered_map<EveType::IdType, EveType> mTypeCache;
        mutable std::unordered_map<EveType::IdType, QString> mTypeNameCache;
        mutable std::unordered_map<EveType::IdType, MarketGroup> mTypeMarketGroupParentCache;
        mutable std::unordered_map<EveType::IdType, MetaGroup> mTypeMetaGroupCache;
        mutable std::unordered_map<quint64, QString> mLocationNameCache;
        mutable std::unordered_map<TypeLocationPair, ItemPrice, boost::hash<TypeLocationPair>> mSellPrices;
        mutable std::unordered_map<TypeLocationPair, ItemPrice, boost::hash<TypeLocationPair>> mBuyPrices;

        std::unordered_map<std::string, ImporterPtr> mItemPriceImporters;

        std::unordered_map<RefType::IdType, QString> mRefTypeNames;

        mutable std::unordered_map<Character::IdType, std::unique_ptr<AssetList>> mCharacterAssets;

        CharacterTimerMap mCharacterUtcCacheTimes;
        CharacterTimerMap mAssetsUtcCacheTimes;
        CharacterTimerMap mWalletJournalUtcCacheTimes;
        CharacterTimerMap mWalletTransactionsUtcCacheTimes;
        CharacterTimerMap mMarketOrdersUtcCacheTimes;

        CharacterTimerMap mCharacterUtcUpdateTimes;
        CharacterTimerMap mAssetsUtcUpdateTimes;
        CharacterTimerMap mWalletJournalUtcUpdateTimes;
        CharacterTimerMap mWalletTransactionsUtcUpdateTimes;
        CharacterTimerMap mMarketOrdersUtcUpdateTimes;

        mutable MarketOrderMap mSellOrders;
        mutable MarketOrderMap mBuyOrders;
        mutable MarketOrderMap mArchivedOrders;

        mutable std::unordered_map<CharacterTypePair, ItemCost, boost::hash<CharacterTypePair>> mItemCostCache;

        void createDb();
        void createDbSchema();
        void precacheRefTypes();
        void precacheRefTypes(const RefTypeRepository::RefTypeList &refs);
        void precacheCacheTimers();
        void deleteOldWalletEntries();

        uint startTask(const QString &description);
        uint startTask(uint parentTask, const QString &description);

        void importCharacter(Character::IdType id, uint parentTask, const Key &key);
        void importItemPrices(const std::string &importerName, const ItemPriceImporter::TypeLocationPairs &target);
        void importMarketLogs(Character::IdType id, MarketOrders &orders);

        Key getCharacterKey(Character::IdType id) const;

        void finishItemPriceImportTask(const QString &info);

        ItemPrice getTypeSellPrice(EveType::IdType id, quint64 stationId, bool dontThrow) const;
        ItemPrice getTypeBuyPrice(EveType::IdType id, quint64 stationId, bool dontThrow) const;
        void computeAssetListSellValue(const AssetList &list) const;
        double getTotalItemSellValue(const Item &item, quint64 locationId) const;

        ItemPrice saveTypePrice(ItemPrice::Type type,
                                quint64 stationId,
                                EveType::IdType typeId,
                                const QDateTime &priceTime,
                                double price) const;

        void saveUpdateTimer(TimerType timer, CharacterTimerMap &map, Character::IdType characterId) const;

        const EveType &getEveType(EveType::IdType id) const;
        const MarketGroup &getMarketGroupParent(MarketGroup::IdType id) const;

        static void showSplashMessage(const QString &message, QSplashScreen &splash);
    };
}
