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
#include <QIcon>

#include "EveDataProvider.h"
#include "AssetProvider.h"
#include "PriceSettings.h"
#include "ExternalOrder.h"
#include "AssetList.h"
#include "IconUtils.h"
#include "TextUtils.h"

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
        return (row >= static_cast<int>(mChildItems.size())) ? (nullptr) : (mChildItems[row].get());
    }

    int AssetModel::TreeItem::childCount() const noexcept
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

    AssetModel::LocationId AssetModel::TreeItem::locationId() const noexcept
    {
        return mLocationId;
    }

    void AssetModel::TreeItem::setLocationId(LocationId id) noexcept
    {
        mLocationId = id;
    }

    QVariant AssetModel::TreeItem::decoration() const
    {
        return mDecoration;
    }

    void AssetModel::TreeItem::setDecoration(const QVariant &data)
    {
        mDecoration = data;
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

    AssetModel::TreeItem *AssetModel::TreeItem::parent() const noexcept
    {
        return mParentItem;
    }

    AssetModel::AssetModel(const AssetProvider &assetProvider,
                           const EveDataProvider &nameProvider,
                           bool corp,
                           QObject *parent)
        : QAbstractItemModel(parent)
        , mAssetProvider(assetProvider)
        , mDataProvider(nameProvider)
        , mCorp(corp)
    {
        mRootItem.setData(QVariantList{}
            << tr("Name")
            << tr("Quantity")
            << tr("Unit volume")
            << tr("Total volume")
            << tr("Local unit sell price")
            << tr("Local total sell price")
            << tr("Owner"));

        connect(&mDataProvider, &EveDataProvider::namesChanged, this, &AssetModel::updateNames);
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

        QLocale locale;

        switch (role) {
        case Qt::DecorationRole:
            if (column == typeColumn)
                return item->decoration();
            break;
        case Qt::UserRole:
            if (column == ownerColumn)
            {
                const auto id = item->data(ownerColumn).value<Character::IdType>();
                return (id == Character::invalidId) ? (QVariant{}) : (mDataProvider.getGenericName(id));
            }

            return item->data(column);
        case Qt::DisplayRole:
            switch (column) {
            case quantityColumn:
                return locale.toString(item->data(quantityColumn).toUInt());
            case unitVolumeColumn:
                if (item->parent() != &mRootItem)
                    return QString{"%1m³"}.arg(locale.toString(item->data(unitVolumeColumn).toDouble(), 'f', 2));
                break;
            case totalVolumeColumn:
                return QString{"%1m³"}.arg(locale.toString(item->data(totalVolumeColumn).toDouble(), 'f', 2));
            case unitPriceColumn:
                if (item->parent() != &mRootItem)
                    return TextUtils::currencyToString(item->data(unitPriceColumn).toDouble(), locale);
                break;
            case totalPriceColumn:
                return TextUtils::currencyToString(item->data(totalPriceColumn).toDouble(), locale);
            case ownerColumn:
                {
                    const auto id = item->data(ownerColumn).value<Character::IdType>();
                    return (id == Character::invalidId) ? (QVariant{}) : (mDataProvider.getGenericName(id));
                }
            }
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
            if ((column == unitPriceColumn || column == totalPriceColumn) && item->parent() != &mRootItem)
            {
                QSettings settings;
                const auto maxPriceAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                if (item->priceTimestamp() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxPriceAge))
                    return QColor{255, 255, 192};
            }
            break;
        case Qt::ToolTipRole:
            if ((column == unitPriceColumn || column == totalPriceColumn) && item->parent() != &mRootItem)
                return tr("Price update time: %1").arg(TextUtils::dateTimeToString(item->priceTimestamp().toLocalTime(), locale));
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
             return QModelIndex{};

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
    }

    void AssetModel::setCustomStation(quint64 id)
    {
        mCustomStationId = id;
    }

    void AssetModel::setCombineCharacters(bool flag)
    {
        mCombineCharacters = flag;
    }

    bool AssetModel::isCombiningCharacters() const
    {
        return mCombineCharacters;
    }

    void AssetModel::reset()
    {
        beginResetModel();

        mRootItem.clearChildren();
        mLocationItems.clear();

        mTotalAssets = 0;
        mTotalVolume = mTotalSellPrice = 0.;

        if (mCombineCharacters)
        {
            const auto assets = (mCorp) ? (mAssetProvider.fetchAllCorpAssets()) : (mAssetProvider.fetchAllAssets());
            for (const auto &list : assets)
                fillAssets(list);
        }
        else if (mCharacterId != Character::invalidId)
        {
            const auto assets = (mCorp) ?
                                (mAssetProvider.fetchCorpAssetsForCharacter(mCharacterId)) :
                                (mAssetProvider.fetchAssetsForCharacter(mCharacterId));
            fillAssets(assets);
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

    AssetModel::LocationId AssetModel::getAssetLocationId(const QModelIndex &index) const
    {
        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        return (item != nullptr) ? (item->locationId()) : (LocationId{});
    }

    void AssetModel::updateNames()
    {
        std::function<void(TreeItem &, const QModelIndex &, int)> updateOwner = [=, &updateOwner](TreeItem &item, const QModelIndex &parent, int row) {
            const auto thisIndex = index(row, ownerColumn, parent);

            emit dataChanged(thisIndex, thisIndex, QVector<int>{} << Qt::UserRole << Qt::DisplayRole);

            const auto childCount = item.childCount();
            for (auto i = 0; i < childCount; ++i)
                updateOwner(*item.child(i), index(row, 0, parent), i);
        };

        updateOwner(mRootItem, QModelIndex{}, 0);
    }

    void AssetModel::buildItemMap(const Item &item, TreeItem &treeItem, LocationId locationId, Character::IdType ownerId)
    {
        for (const auto &child : item)
        {
            auto childItem = createTreeItemForItem(*child, locationId, ownerId);
            buildItemMap(*child, *childItem, locationId, ownerId);

            treeItem.appendChild(std::move(childItem));
        }
    }

    std::unique_ptr<AssetModel::TreeItem> AssetModel
    ::createTreeItemForItem(const Item &item, LocationId locationId, Character::IdType ownerId)
    {
        const auto typeId = item.getTypeId();
        const auto volume = mDataProvider.getTypeVolume(typeId);
        const auto quantity = item.getQuantity();
        const auto sellPrice = (item.isBPC(mDataProvider)) ? (ExternalOrder::nullOrder()) : (mDataProvider.getTypeStationSellPrice(typeId, locationId));
        const auto metaIcon = IconUtils::getIconForMetaGroup(mDataProvider.getTypeMetaGroupName(typeId));

        mTotalAssets += quantity;
        mTotalVolume += volume * quantity;
        mTotalSellPrice += sellPrice->getPrice() * quantity;

        auto treeItem = std::make_unique<TreeItem>();
        treeItem->setData(QVariantList{}
            << mDataProvider.getTypeName(typeId)
            << quantity
            << volume
            << (volume * quantity)
            << sellPrice->getPrice()
            << (sellPrice->getPrice() * quantity)
            << ownerId
        );
        treeItem->setPriceTimestamp(sellPrice->getUpdateTime());
        treeItem->setLocationId(locationId);

        if (!metaIcon.isNull())
            treeItem->setDecoration(metaIcon);

        return treeItem;
    }

    void AssetModel::fillAssets(const std::shared_ptr<AssetList> &assets)
    {
        for (const auto &item : *assets)
        {
            auto id = item->getLocationId();
            if (!id)
                id = LocationId{};

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
                    << 0.
                    << Character::invalidId);
                treeItem->setLocationId(*id);
                locationItem = treeItem.get();

                mLocationItems[*id] = locationItem;
                mRootItem.appendChild(std::move(treeItem));
            }
            else
            {
                locationItem = mLocationItems[*id];
            }

            const auto locationId = (mCustomStationId == 0) ? (*id) : (mCustomStationId);

            const auto curAssets = mTotalAssets;
            const auto curVolume = mTotalVolume;
            const auto curSellPrice = mTotalSellPrice;

            auto treeItem = createTreeItemForItem(*item, locationId, assets->getCharacterId());

            buildItemMap(*item, *treeItem, locationId, assets->getCharacterId());
            locationItem->appendChild(std::move(treeItem));

            auto data = locationItem->data();
            data[quantityColumn] = data[quantityColumn].toUInt() + mTotalAssets - curAssets;
            data[totalVolumeColumn] = data[totalVolumeColumn].toDouble() + mTotalVolume - curVolume;
            data[totalPriceColumn] = data[totalPriceColumn].toDouble() + mTotalSellPrice - curSellPrice;
            locationItem->setData(data);
        }
    }
}
