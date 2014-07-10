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
#include <QSettings>
#include <QLocale>
#include <QColor>
#include <QFont>
#include <QDebug>

#include "EveDataProvider.h"
#include "AssetProvider.h"
#include "PriceSettings.h"
#include "ItemPrice.h"

#include "AssetModel.h"

namespace Evernus
{
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

    QVariantList AssetModel::TreeItem::data() const
    {
        return mItemData;
    }

    void AssetModel::TreeItem::setData(const QVariantList &data)
    {
        mItemData = data;
    }

    QDateTime AssetModel::TreeItem::priceTimestamp() const
    {
        return mPriceTimestamp;
    }

    void AssetModel::TreeItem::setPriceTimestamp(const QDateTime &dt)
    {
        mPriceTimestamp = dt;
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

    AssetModel::AssetModel(const AssetProvider &assetProvider, const EveDataProvider &nameProvider, QObject *parent)
        : QAbstractItemModel{parent}
        , mAssetProvider{assetProvider}
        , mDataProvider{nameProvider}
    {
        mRootItem.setData(QVariantList{}
            << "Name"
            << "Quantity"
            << "Unit volume"
            << "Total volume"
            << "Local unit sell price"
            << "Local total sell price");
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

         auto item = static_cast<const TreeItem *>(index.internalPointer());

         const auto column = index.column();

         switch (role) {
         case Qt::DisplayRole:
             return item->data(column);
         case Qt::FontRole:
             if (item->parent() == &mRootItem)
             {
                 QFont font;
                 font.setBold(true);

                 return font;
             }

             return QFont{};
         case Qt::BackgroundRole:
             if ((column == 4 || column == 5) && item->parent() != &mRootItem)
             {
                 QSettings settings;
                 const auto maxPriceAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                 if (item->priceTimestamp() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxPriceAge))
                     return QColor{255, 255, 192};
             }
             break;
         case Qt::ToolTipRole:
             if ((column == 4 || column == 5) && item->parent() != &mRootItem)
                 return tr("Price update time: %1").arg(item->priceTimestamp().toLocalTime().toString());
         }

         return QVariant{};
    }

    QVariant AssetModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
            return mRootItem.data(section);

        return QVariant{};
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

        mTotalAssets = 0;
        mTotalVolume = mTotalSellPrice = 0.;

        if (mCharacterId != Character::invalidId)
        {
            const auto &assets = mAssetProvider.fetchForCharacter(mCharacterId);
            for (const auto &item : assets)
            {
                auto id = item->getLocationId();
                if (!id)
                    id = ItemData::LocationIdType::value_type{};

                TreeItem *locationItem = nullptr;

                const auto it = mLocationItems.find(*id);
                if (it == std::end(mLocationItems))
                {
                    auto treeItem = std::make_unique<TreeItem>();
                    treeItem->setData(QVariantList{}
                        << mDataProvider.getLocationName(*id)
                        << 0
                        << QString{}
                        << 0.
                        << QString{}
                        << 0.);
                    locationItem = treeItem.get();

                    mLocationItems[*id] = locationItem;
                    mRootItem.appendChild(std::move(treeItem));
                }
                else
                {
                    locationItem = mLocationItems[*id];
                }

                auto treeItem = createTreeItemForItem(*item, *id);

                const auto curAssets = mTotalAssets;
                const auto curVolume = mTotalVolume;
                const auto curSellPrice = mTotalSellPrice;

                buildItemMap(*item, *treeItem, *id);
                locationItem->appendChild(std::move(treeItem));

                auto data = locationItem->data();
                data[1] = data[1].toUInt() + mTotalAssets - curAssets;
                data[3] = data[3].toDouble() + mTotalVolume - curVolume;
                data[5] = data[5].toDouble() + mTotalSellPrice - curSellPrice;
                locationItem->setData(data);
            }
        }

        QLocale locale;

        for (auto &item : mLocationItems)
        {
            auto data = item.second->data();
            data[1] = locale.toString(data[1].toUInt());
            data[3] = QString{"%1m³"}.arg(locale.toString(data[3].toDouble(), 'f', 2));
            data[5] = locale.toCurrencyString(data[5].toDouble(), "ISK");
            item.second->setData(data);
        }

        endResetModel();
    }

    uint AssetModel::getTotalAssets() const noexcept
    {
        return mTotalAssets;
    }

    double AssetModel::getTotalVolume() const noexcept
    {
        return mTotalVolume;
    }

    double AssetModel::getTotalSellPrice() const noexcept
    {
        return mTotalSellPrice;
    }

    void AssetModel::buildItemMap(const Item &item, TreeItem &treeItem, ItemData::LocationIdType::value_type locationId)
    {
        const auto quantity = item.getQuantity();
        const auto typeId = item.getTypeId();

        mTotalAssets += quantity;
        mTotalVolume += mDataProvider.getTypeVolume(typeId) * quantity;
        mTotalSellPrice += mDataProvider.getTypeSellPrice(typeId, locationId).getValue() * quantity;

        for (const auto &child : item)
        {
            auto childItem = createTreeItemForItem(*child, locationId);
            buildItemMap(*child, *childItem, locationId);

            treeItem.appendChild(std::move(childItem));
        }
    }

    std::unique_ptr<AssetModel::TreeItem> AssetModel
    ::createTreeItemForItem(const Item &item, ItemData::LocationIdType::value_type locationId) const
    {
        QLocale locale;

        const auto typeId = item.getTypeId();
        const auto volume = mDataProvider.getTypeVolume(typeId);
        const auto quantity = item.getQuantity();
        const auto sellPrice = mDataProvider.getTypeSellPrice(typeId, locationId);

        auto treeItem = std::make_unique<TreeItem>();
        treeItem->setData(QVariantList{}
            << mDataProvider.getTypeName(typeId)
            << locale.toString(quantity)
            << QString{"%1m³"}.arg(locale.toString(volume, 'f', 2))
            << QString{"%1m³"}.arg(locale.toString(volume * quantity, 'f', 2))
            << locale.toCurrencyString(sellPrice.getValue(), "ISK")
            << locale.toCurrencyString(sellPrice.getValue() * quantity, "ISK")
        );
        treeItem->setPriceTimestamp(sellPrice.getUpdateTime());

        return treeItem;
    }
}
