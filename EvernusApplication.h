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

#include "ConquerableStationRepository.h"
#include "AssetValueSnapshotRepository.h"
#include "ItemPriceImporterRegistry.h"
#include "WalletSnapshotRepository.h"
#include "CharacterRepository.h"
#include "AssetListRepository.h"
#include "ItemPriceRepository.h"
#include "ItemPriceImporter.h"
#include "EveTypeRepository.h"
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
    {
        Q_OBJECT

    public:
        EvernusApplication(int &argc, char *argv[]);
        virtual ~EvernusApplication() = default;

        virtual QString getTypeName(EveType::IdType id) const override;
        virtual double getTypeVolume(EveType::IdType id) const override;
        virtual ItemPrice getTypeSellPrice(EveType::IdType id, quint64 stationId) const override;

        virtual QString getLocationName(quint64 id) const override;

        virtual void registerImporter(const std::string &name, std::unique_ptr<ItemPriceImporter> &&importer) override;

        virtual const AssetList &fetchForCharacter(Character::IdType id) const override;

        const KeyRepository &getKeyRepository() const noexcept;
        const CharacterRepository &getCharacterRepository() const noexcept;
        const WalletSnapshotRepository &getWalletSnapshotRepository() const noexcept;
        const AssetValueSnapshotRepository &getAssetValueSnapshotRepository() const noexcept;

        APIManager &getAPIManager() noexcept;

    signals:
        void taskStarted(uint taskId, const QString &description);
        void taskStarted(uint taskId, uint parentTask, const QString &description);
        void taskStatusChanged(uint taskId, const QString &error);

        void apiError(const QString &info);

        void conquerableStationsChanged();
        void charactersChanged();
        void assetsChanged();
        void iskChanged();
        void itemPricesChanged();

    public slots:
        void refreshCharacters();
        void refreshCharacter(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshAssets(Character::IdType id, uint parentTask = TaskConstants::invalidTask);
        void refreshConquerableStations();
        void refreshItemPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &target);

    private slots:
        void scheduleCharacterUpdate();
        void updateCharacters();

        void showPriceImportError(const QString &info);
        void updateItemPrices(const std::vector<ItemPrice> &prices);

    private:
        typedef std::pair<EveType::IdType, quint64> TypeLocationPair;

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

        mutable std::unordered_map<Character::IdType, std::unique_ptr<AssetList>> mCharacterAssets;

        void createDb();
        void createDbSchema();

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
