#pragma once

#include <unordered_map>

#include <QAbstractItemModel>

#include "Character.h"
#include "Item.h"

namespace Evernus
{
    class AssetListRepository;
    class NameProvider;
    class AssetList;

    class AssetModel
        : public QAbstractItemModel
    {
    public:
        AssetModel(const AssetListRepository &assetRepository, const NameProvider &nameProvider, QObject *parent = nullptr);
        virtual ~AssetModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex{}) const override;
        virtual QModelIndex parent(const QModelIndex &index) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        void setCharacter(Character::IdType id);

        void reset();

    private:
        class TreeItem
        {
        public:
            TreeItem() = default;
            explicit TreeItem(const QVariantList &data);
            ~TreeItem() = default;

            void appendChild(std::unique_ptr<TreeItem> &&child);
            void clearChildren();

            TreeItem *child(int row) const;
            int childCount() const;
            int columnCount() const;
            QVariant data(int column) const;
            int row() const;
            TreeItem *parent() const;

        private:
            std::vector<std::unique_ptr<TreeItem>> mChildItems;
            QVariantList mItemData;
            TreeItem *mParentItem = nullptr;
        };

        const AssetListRepository &mAssetRepository;
        const NameProvider &mNameProvider;

        Character::IdType mCharacterId = Character::invalidId;

        TreeItem mRootItem;

        std::unordered_map<ItemData::LocationIdType::value_type, TreeItem *> mLocationItems;

        void buildItemMap(const Item &item, TreeItem &treeItem);

        std::unique_ptr<TreeItem> createTreeItemForItem(const Item &item) const;
    };
}
