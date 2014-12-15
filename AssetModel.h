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

#include <QAbstractItemModel>
#include <QDateTime>

#include "Character.h"
#include "Item.h"

namespace Evernus
{
    class EveDataProvider;
    class AssetProvider;
    class AssetList;

    class AssetModel
        : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        using LocationId = ItemData::LocationIdType::value_type;

        AssetModel(const AssetProvider &assetProvider, const EveDataProvider &dataProvider, QObject *parent = nullptr);
        virtual ~AssetModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex{}) const override;
        virtual QModelIndex parent(const QModelIndex &index) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        void setCharacter(Character::IdType id);
        void setCustomStation(quint64 id);

        void reset();

        uint getTotalAssets() const noexcept;
        double getTotalVolume() const noexcept;
        double getTotalSellPrice() const noexcept;

        LocationId getAssetLocationId(const QModelIndex &index) const;

    private:
        class TreeItem
        {
        public:
            TreeItem() = default;
            ~TreeItem() = default;

            void appendChild(std::unique_ptr<TreeItem> &&child);
            void clearChildren();

            TreeItem *child(int row) const;

            int childCount() const;
            int columnCount() const;

            QVariant data(int column) const;
            QVariantList data() const;
            void setData(const QVariantList &data);

            QDateTime priceTimestamp() const;
            void setPriceTimestamp(const QDateTime &dt);

            LocationId locationId() const noexcept;
            void setLocationId(LocationId id) noexcept;

            QVariant decoration() const;
            void setDecoration(const QVariant &data);

            int row() const;

            TreeItem *parent() const;

        private:
            std::vector<std::unique_ptr<TreeItem>> mChildItems;
            QVariantList mItemData;
            QDateTime mPriceTimestamp;
            LocationId mLocationId = LocationId{};
            QVariant mDecoration;
            TreeItem *mParentItem = nullptr;
        };

        static const auto typeColumn = 0;
        static const auto quantityColumn = 1;
        static const auto unitVolumeColumn = 2;
        static const auto totalVolumeColumn = 3;
        static const auto unitPriceColumn = 4;
        static const auto totalPriceColumn = 5;

        const AssetProvider &mAssetProvider;
        const EveDataProvider &mDataProvider;

        Character::IdType mCharacterId = Character::invalidId;
        quint64 mCustomStationId = 0;

        TreeItem mRootItem;

        uint mTotalAssets = 0;
        double mTotalVolume = 0.;
        double mTotalSellPrice = 0.;

        std::unordered_map<LocationId, TreeItem *> mLocationItems;

        void buildItemMap(const Item &item, TreeItem &treeItem, LocationId locationId);

        std::unique_ptr<TreeItem> createTreeItemForItem(const Item &item, LocationId locationId) const;
    };
}
