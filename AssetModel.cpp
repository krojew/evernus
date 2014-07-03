#include "AssetListRepository.h"
#include "NameProvider.h"

#include "AssetModel.h"

namespace Evernus
{
    AssetModel::TreeItem::TreeItem(const QList<QVariant> &data)
        : mItemData{data}
    {
    }

    void AssetModel::TreeItem::appendChild(std::unique_ptr<TreeItem> &&child)
    {
        child->mParentItem = this;
        mChildItems.emplace_back(std::move(child));
    }

    void AssetModel::TreeItem::clearChildren()
    {
        mChildItems.clear();
    }

    AssetModel::TreeItem *AssetModel::TreeItem::child(int row) const
    {
        return (row >= mChildItems.size()) ? (nullptr) : (mChildItems[row].get());
    }

    int AssetModel::TreeItem::childCount() const
    {
        return static_cast<int>(mChildItems.size());
    }

    int AssetModel::TreeItem::columnCount() const
    {
        return mItemData.count();
    }

    QVariant AssetModel::TreeItem::data(int column) const
    {
        return mItemData.value(column);
    }

    int AssetModel::TreeItem::row() const
    {
        if (mParentItem != nullptr)
        {
            auto row = 0;
            for (const auto &child : mParentItem->mChildItems)
            {
                if (child.get() == this)
                    return row;

                ++row;
            }
        }

        return 0;
    }

    AssetModel::TreeItem *AssetModel::TreeItem::parent() const
    {
        return mParentItem;
    }

    AssetModel::AssetModel(const AssetListRepository &assetRepository, const NameProvider &nameProvider, QObject *parent)
        : QAbstractItemModel{parent}
        , mAssetRepository{assetRepository}
        , mNameProvider{nameProvider}
        , mRootItem{QVariantList{} << "name" << "quantity" << "size"}
    {
    }

    int AssetModel::columnCount(const QModelIndex &parent) const
    {
        if (parent.isValid())
             return static_cast<const TreeItem *>(parent.internalPointer())->columnCount();
         else
             return mRootItem.columnCount();
    }

    QVariant AssetModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
             return QVariant{};

         if (role != Qt::DisplayRole)
             return QVariant{};

         auto item = static_cast<const TreeItem *>(index.internalPointer());
         return item->data(index.column());
    }

    QModelIndex AssetModel::index(int row, int column, const QModelIndex &parent) const
    {
        if (!hasIndex(row, column, parent))
             return QModelIndex();

         const TreeItem *parentItem = nullptr;

         if (!parent.isValid())
             parentItem = &mRootItem;
         else
             parentItem = static_cast<const TreeItem *>(parent.internalPointer());

         auto childItem = parentItem->child(row);
         if (childItem)
             return createIndex(row, column, childItem);

         return QModelIndex{};
    }

    QModelIndex AssetModel::parent(const QModelIndex &index) const
    {
        if (!index.isValid())
             return QModelIndex{};

         auto childItem = static_cast<const TreeItem *>(index.internalPointer());
         auto parentItem = childItem->parent();

         if (parentItem == &mRootItem)
             return QModelIndex{};

         return createIndex(parentItem->row(), 0, parentItem);
    }

    int AssetModel::rowCount(const QModelIndex &parent) const
    {
         const TreeItem *parentItem = nullptr;
         if (parent.column() > 0)
             return 0;

         if (!parent.isValid())
             parentItem = &mRootItem;
         else
             parentItem = static_cast<const TreeItem *>(parent.internalPointer());

         return parentItem->childCount();
    }

    void AssetModel::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        reset();
    }

    void AssetModel::reset()
    {
        beginResetModel();

        mRootItem.clearChildren();
        mLocationItems.clear();

        if (mCharacterId != Character::invalidId)
        {
            const auto assets = mAssetRepository.fetchForCharacter(mCharacterId);
            for (const auto &item : assets)
            {
                auto id = item->getLocationId();
                if (!id)
                    id = ItemData::LocationIdType::value_type{};

                TreeItem *locationItem = nullptr;

                const auto it = mLocationItems.find(*id);
                if (it == std::end(mLocationItems))
                {
                    auto treeItem = std::make_unique<TreeItem>(QVariantList{}
                        << mNameProvider.getLocationName(*id));
                    locationItem = treeItem.get();

                    mLocationItems[*id] = locationItem;
                    mRootItem.appendChild(std::move(treeItem));
                }
                else
                {
                    locationItem = mLocationItems[*id];
                }

                auto treeItem = createTreeItemForItem(*item);

                buildItemMap(*item, *treeItem);
                locationItem->appendChild(std::move(treeItem));
            }
        }

        endResetModel();
    }

    void AssetModel::buildItemMap(const Item &item, TreeItem &treeItem)
    {
        for (const auto &child : item)
        {
            auto childItem = createTreeItemForItem(*child);
            buildItemMap(*child, *childItem);

            treeItem.appendChild(std::move(childItem));
        }
    }

    std::unique_ptr<AssetModel::TreeItem> AssetModel::createTreeItemForItem(const Item &item) const
    {
        auto treeItem = std::make_unique<TreeItem>(QVariantList()
            << mNameProvider.getTypeName(item.getTypeId())
        );

        return treeItem;
    }
}
