#pragma once

#include <unordered_map>

#include <QAbstractItemModel>

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
    public:
        AssetModel(const AssetProvider &assetProvider, const EveDataProvider &dataProvider, QObject *parent = nullptr);
        virtual ~AssetModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex{}) const override;
        virtual QModelIndex parent(const QModelIndex &index) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        void setCharacter(Character::IdType id);

        void reset();

        uint getTotalAssets() const noexcept;
        double getTotalVolume() const noexcept;
        double getTotalSellPrice() const noexcept;

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
            int row() const;
            TreeItem *parent() const;

        private:
            std::vector<std::unique_ptr<TreeItem>> mChildItems;
            QVariantList mItemData;
            TreeItem *mParentItem = nullptr;
        };

        const AssetProvider &mAssetProvider;
        const EveDataProvider &mDataProvider;

        Character::IdType mCharacterId = Character::invalidId;

        TreeItem mRootItem;

        uint mTotalAssets = 0;
        double mTotalVolume = 0.;
        double mTotalSellPrice = 0.;

        std::unordered_map<ItemData::LocationIdType::value_type, TreeItem *> mLocationItems;

        void buildItemMap(const Item &item, TreeItem &treeItem, ItemData::LocationIdType::value_type locationId);

        std::unique_ptr<TreeItem> createTreeItemForItem(const Item &item, ItemData::LocationIdType::value_type locationId) const;
    };
}
