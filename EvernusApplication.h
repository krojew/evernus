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

#include <unordered_map>
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
#include "CacheTimerRepository.h"
#include "CharacterRepository.h"
#include "AssetListRepository.h"
#include "ItemPriceRepository.h"
#include "ItemCostRepository.h"
#include "CacheTimerProvider.h"
#include "ItemPriceImporter.h"
#include "EveTypeRepository.h"
#include "RefTypeRepository.h"
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
    {
        Q_OBJECT

    public:
        EvernusApplication(int &argc, char *argv[]);
        virtual ~EvernusApplication() = default;

        virtual QString getTypeName(EveType::IdType id) const override;
        virtual double getTypeVolume(EveType::IdType id) const override;
        virtual ItemPrice getTypeSellPrice(EveType::IdType id, quint64 stationId) const override;

        virtual QString getLocationName(quint64 id) const override;

        virtual QString getRefTypeName(uint id) const override;

        virtual void registerImporter(const std::string &name, std::unique_ptr<ItemPriceImporter> &&importer) override;

        virtual const AssetList &fetchForCharacter(Character::IdType id) const override;

        virtual QDateTime getLocalCacheTimer(Character::IdType id, TimerType type) const override;
        virtual void setUtcCacheTimer(Character::IdType id, TimerType type, const QDateTime &dt) override;

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
        void refreshMarketOrders(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshConquerableStations();
        void refreshItemPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &target);
        void refreshItemPricesFromFile(const ItemPriceImporter::TypeLocationPairs &target);

    private slots:
        void scheduleCharacterUpdate();
        void updateCharacters();

        void showPriceImportError(const QString &info);
        void updateItemPrices(const std::vector<ItemPrice> &prices);

    private:
        typedef std::pair<EveType::IdType, quint64> TypeLocationPair;
        typedef std::unordered_map<Character::IdType, QDateTime> CacheTimerMap;

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
        std::unique_ptr<WalletTransactionRepository> mWalletTransactionRepository;
        std::unique_ptr<MarketOrderRepository> mMarketOrderRepository;
        std::unique_ptr<ItemCostRepository> mItemCostRepository;
        std::unique_ptr<MarketOrderValueSnapshotRepository> mMarketOrderValueSnapshotRepository;
        std::unique_ptr<EveTypeRepository> mEveTypeRepository;

        APIManager mAPIManager;

        uint mTaskId = TaskConstants::invalidTask + 1;
        uint mCurrentItemPriceImportTask = TaskConstants::invalidTask;

        bool mCharacterUpdateScheduled = false;

        mutable std::unordered_map<EveType::IdType, QString> mTypeNameCache;
        mutable std::unordered_map<EveType::IdType, double> mTypeVolumeCache;
        mutable std::unordered_map<quint64, QString> mLocationNameCache;
        mutable std::unordered_map<TypeLocationPair, ItemPrice, boost::hash<TypeLocationPair>> mSellPrices;

        std::unordered_map<std::string, ImporterPtr> mItemPriceImporters;

        std::unordered_map<RefType::IdType, QString> mRefTypeNames;

        mutable std::unordered_map<Character::IdType, std::unique_ptr<AssetList>> mCharacterAssets;

        CacheTimerMap mCharacterLocalCacheTimes;
        CacheTimerMap mAssetsLocalCacheTimes;
        CacheTimerMap mWalletJournalLocalCacheTimes;
        CacheTimerMap mWalletTransactionsLocalCacheTimes;
        CacheTimerMap mMarketOrdersLocalCacheTimes;

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

        Key getCharacterKey(Character::IdType id) const;

        void finishItemPriceImportTask(const QString &info);

        ItemPrice getTypeSellPrice(EveType::IdType id, quint64 stationId, bool dontThrow) const;
        void computeAssetListSellValue(const AssetList &list) const;
        double getTotalItemSellValue(const Item &item, quint64 locationId) const;

        static void showSplashMessage(const QString &message, QSplashScreen &splash);
    };
}
