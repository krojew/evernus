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
#include <unordered_map>

#include <QSettings>
#include <QLocale>
#include <QColor>
#include <QIcon>
#include <QFont>

#include "MarketOrderProvider.h"
#include "CacheTimerProvider.h"
#include "ItemCostProvider.h"
#include "EveDataProvider.h"
#include "PriceSettings.h"
#include "ItemPrice.h"
#include "IconUtils.h"
#include "ItemCost.h"

#include "MarketOrderSellModel.h"

namespace Evernus
{
    void MarketOrderSellModel::TreeItem::appendChild(std::unique_ptr<TreeItem> &&child)
    {
        child->mParentItem = this;
        mChildItems.emplace_back(std::move(child));
    }

    void MarketOrderSellModel::TreeItem::clearChildren()
    {
        mChildItems.clear();
    }

    MarketOrderSellModel::TreeItem *MarketOrderSellModel::TreeItem::child(int row) const
    {
        return (row >= mChildItems.size()) ? (nullptr) : (mChildItems[row].get());
    }

    int MarketOrderSellModel::TreeItem::childCount() const
    {
        return static_cast<int>(mChildItems.size());
    }

    const MarketOrder *MarketOrderSellModel::TreeItem::getOrder() const noexcept
    {
        return mOrder;
    }

    void MarketOrderSellModel::TreeItem::setOrder(const MarketOrder *order) noexcept
    {
        mOrder = order;
    }

    QString MarketOrderSellModel::TreeItem::getGroupName() const
    {
        return mGroupName;
    }

    void MarketOrderSellModel::TreeItem::setGroupName(const QString &name)
    {
        mGroupName = name;
    }

    void MarketOrderSellModel::TreeItem::setGroupName(QString &&name)
    {
        mGroupName = std::move(name);
    }

    int MarketOrderSellModel::TreeItem::row() const
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

    MarketOrderSellModel::TreeItem *MarketOrderSellModel::TreeItem::parent() const
    {
        return mParentItem;
    }

    MarketOrderSellModel::MarketOrderSellModel(const MarketOrderProvider &orderProvider,
                                               const EveDataProvider &dataProvider,
                                               const ItemCostProvider &itemCostProvider,
                                               const CacheTimerProvider &cacheTimerProvider,
                                               QObject *parent)
        : MarketOrderModel{parent}
        , mOrderProvider{orderProvider}
        , mDataProvider{dataProvider}
        , mItemCostProvider{itemCostProvider}
        , mCacheTimerProvider{cacheTimerProvider}
    {
    }

    int MarketOrderSellModel::columnCount(const QModelIndex &parent) const
    {
        return 16;
    }

    QVariant MarketOrderSellModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant{};

        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        const auto column = index.column();

        if (mGrouping != Grouping::None)
        {
            if (item->parent() == &mRootItem)
            {
                if (role == Qt::UserRole || (role == Qt::DisplayRole && column == nameColumn))
                    return item->getGroupName();

                return QVariant{};
            }
        }

        const auto data = item->getOrder();

        const auto secondsToString = [this](auto duration) {
            QString res;

            duration /= 60;

            const auto minutes = duration % 60;
            duration /= 60;

            const auto hours = duration % 24;
            const auto days = duration / 24;

            if (hours == 0 && days == 0)
                return res.sprintf(tr("%02dmin").toLatin1().data(), minutes);

            if (days == 0)
                return res.sprintf(tr("%02dh %02dmin").toLatin1().data(), hours, minutes);

            return res.sprintf(tr("%dd %02dh").toLatin1().data(), days, hours);
        };

        switch (role) {
        case Qt::ToolTipRole:
            if (column == priceColumn)
            {
                const auto price = mDataProvider.getTypeSellPrice(data->getTypeId(), data->getLocationId());
                if (price.isNew())
                    return tr("No price data-> Please import prices from Assets tab or by using Margin tool.");

                QLocale locale;

                if (price.getValue() < data->getPrice())
                {
                    return tr("You have been undercut. Current price is %1 (%2 different from yours).\nClick the icon for details.")
                        .arg(locale.toCurrencyString(price.getValue(), "ISK"))
                        .arg(locale.toCurrencyString(price.getValue() - data->getPrice(), "ISK"));
                }

                QSettings settings;
                const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                if (price.getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                {
                    return tr("Price data is too old (valid on %1).\nPlease import prices from Assets tab or by using Margin tool.")
                        .arg(locale.toString(price.getUpdateTime().toLocalTime()));
                }

                return tr("Your price was best on %1").arg(locale.toString(price.getUpdateTime().toLocalTime()));
            }
            break;
        case Qt::DecorationRole:
            if (column == priceColumn)
            {
                const auto price = mDataProvider.getTypeSellPrice(data->getTypeId(), data->getLocationId());
                if (price.isNew())
                    return QIcon{":/images/error.png"};

                if (price.getValue() < data->getPrice())
                    return QIcon{":/images/exclamation.png"};

                QSettings settings;
                const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                if (price.getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                    return QIcon{":/images/error.png"};

                return QIcon{":/images/accept.png"};
            }
            else if (column == nameColumn)
            {
                const auto metaIcon = IconUtils::getIconForMetaGroup(mDataProvider.getTypeMetaGroupName(data->getTypeId()));
                if (!metaIcon.isNull())
                    return metaIcon;
            }
            break;
        case Qt::UserRole:
            switch (column) {
            case nameColumn:
                return mDataProvider.getTypeName(data->getTypeId());
            case groupColumn:
                return mDataProvider.getTypeMarketGroupParentName(data->getTypeId());
            case statusColumn:
                return static_cast<int>(data->getState());
            case customCostColumn:
                return mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId()).getCost();
            case priceColumn:
                return data->getPrice();
            case priceStatusColumn:
                {
                    const auto price = mDataProvider.getTypeSellPrice(data->getTypeId(), data->getLocationId());
                    if (price.isNew())
                        return 0;

                    QSettings settings;
                    const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                    if (price.getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                        return 1;

                    return 2;
                }
            case volumeColumn:
                return QVariantList{} << data->getVolumeRemaining() << data->getVolumeEntered();
            case totalColumn:
                return data->getVolumeRemaining() * data->getPrice();
            case deltaColumn:
                return data->getDelta();
            case profitColumn:
                {
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                    return data->getVolumeRemaining() * (data->getPrice() - cost.getCost());
                }
            case totalProfitColumn:
                {
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                    return data->getVolumeEntered() * (data->getPrice() - cost.getCost());
                }
            case profitPerItemColumn:
                {
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                    return data->getPrice() - cost.getCost();
                }
            case timeLeftColumn:
                {
                    const auto timeEnd = data->getIssued().addDays(data->getDuration()).toMSecsSinceEpoch() / 1000;
                    const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                    if (timeEnd > timeCur)
                        return timeEnd - timeCur;
                }
                break;
            case orderAgeColumn:
                {
                    const auto timeStart = data->getIssued().toMSecsSinceEpoch() / 1000;
                    const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                    if (timeCur > timeStart)
                        return timeCur - timeStart;
                }
                break;
            case firstSeenColumn:
                return data->getFirstSeen();
            case stationColumn:
                return mDataProvider.getLocationName(data->getLocationId());
            }
            break;
        case Qt::DisplayRole:
            {
                const char * const stateNames[] = { "Active", "Closed", "Fulfilled", "Cancelled", "Pending", "Character Deleted" };

                QLocale locale;

                switch (column) {
                case nameColumn:
                    return mDataProvider.getTypeName(data->getTypeId());
                case groupColumn:
                    return mDataProvider.getTypeMarketGroupParentName(data->getTypeId());
                case statusColumn:
                    {
                        const auto prefix = (data->getDelta() != 0) ? ("*") : ("");

                        if (data->getState() == MarketOrder::State::Fulfilled && data->getVolumeRemaining() > 0)
                            return tr("Expired");

                        if ((data->getState() >= MarketOrder::State::Active && data->getState() <= MarketOrder::State::CharacterDeleted))
                            return prefix + tr(stateNames[static_cast<size_t>(data->getState())]);

                        if (data->getState() == MarketOrder::State::Archived)
                            return tr("Archived");
                    }
                    break;
                case customCostColumn:
                    {
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                        if (!cost.isNew())
                            return locale.toCurrencyString(cost.getCost(), "ISK");
                    }
                    break;
                case priceColumn:
                    return locale.toCurrencyString(data->getPrice(), "ISK");
                case priceStatusColumn:
                    {
                        const auto price = mDataProvider.getTypeSellPrice(data->getTypeId(), data->getLocationId());
                        if (price.isNew())
                            return tr("No price data");

                        QSettings settings;
                        const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                        if (price.getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                            return tr("Data too old");
                    }
                    break;
                case volumeColumn:
                    return QString{"%1/%2"}.arg(locale.toString(data->getVolumeRemaining())).arg(locale.toString(data->getVolumeEntered()));
                case totalColumn:
                    return locale.toCurrencyString(data->getVolumeRemaining() * data->getPrice(), "ISK");
                case deltaColumn:
                    if (data->getDelta() != 0)
                        return locale.toString(data->getDelta());
                    break;
                case profitColumn:
                    {
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                        return locale.toCurrencyString(data->getVolumeRemaining() * (data->getPrice() - cost.getCost()), "ISK");
                    }
                case totalProfitColumn:
                    {
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                        return locale.toCurrencyString(data->getVolumeEntered() * (data->getPrice() - cost.getCost()), "ISK");
                    }
                case profitPerItemColumn:
                    {
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                        return locale.toCurrencyString(data->getPrice() - cost.getCost(), "ISK");
                    }
                case timeLeftColumn:
                    {
                        const auto timeEnd = data->getIssued().addDays(data->getDuration()).toMSecsSinceEpoch() / 1000;
                        const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                        if (timeEnd > timeCur)
                            return secondsToString(timeEnd - timeCur);
                    }
                    break;
                case orderAgeColumn:
                    {
                        const auto timeStart = data->getIssued().toMSecsSinceEpoch() / 1000;
                        const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                        if (timeCur > timeStart)
                            return secondsToString(timeCur - timeStart);
                    }
                    break;
                case firstSeenColumn:
                    return locale.toString(data->getFirstSeen().toLocalTime());
                case stationColumn:
                    return mDataProvider.getLocationName(data->getLocationId());
                }
            }
            break;
        case Qt::FontRole:
            if ((column == statusColumn && data->getDelta() != 0) || (column == statusColumn && data->getState() == MarketOrder::State::Fulfilled))
            {
                QFont font;
                font.setBold(true);

                return font;
            }
            break;
        case Qt::BackgroundRole:
            if (column == priceColumn)
            {
                const auto price = mDataProvider.getTypeSellPrice(data->getTypeId(), data->getLocationId());
                if (!price.isNew())
                {
                    if (price.getValue() < data->getPrice())
                        return QColor{255, 192, 192};

                    QSettings settings;
                    const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                    if (price.getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                        return QColor{255, 255, 192};
                }
            }
            else if (column == firstSeenColumn)
            {
                QSettings settings;
                const auto maxAge = settings.value(PriceSettings::marketOrderMaxAgeKey, PriceSettings::marketOrderMaxAgeDefault).toInt();
                if (data->getFirstSeen() < QDateTime::currentDateTimeUtc().addDays(-maxAge))
                    return QColor{255, 255, 192};
            }
            break;
        case Qt::ForegroundRole:
            switch (column) {
            case statusColumn:
                switch (data->getState()) {
                case MarketOrder::State::Active:
                    return QColor{Qt::darkGreen};
                case MarketOrder::State::Closed:
                    return QColor{Qt::gray};
                case MarketOrder::State::Pending:
                    return QColor{Qt::cyan};
                case MarketOrder::State::Cancelled:
                case MarketOrder::State::CharacterDeleted:
                    return QColor{Qt::red};
                case MarketOrder::State::Fulfilled:
                    return (data->getState() == MarketOrder::State::Fulfilled && data->getVolumeRemaining() > 0) ?
                           (QColor{Qt::red}) :
                           (QColor{0, 64, 0});
                default:
                    break;
                }
                break;
            case priceStatusColumn:
                return QColor{Qt::darkRed};
            case profitColumn:
            case totalProfitColumn:
            case profitPerItemColumn:
                return QColor{Qt::darkGreen};
            }
            break;
        case Qt::TextAlignmentRole:
            if (column == priceStatusColumn)
                return Qt::AlignHCenter;
            if (column == volumeColumn || column == deltaColumn)
                return Qt::AlignRight;
        }

        return QVariant{};
    }

    QVariant MarketOrderSellModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case nameColumn:
                return tr("Name");
            case groupColumn:
                return tr("Group");
            case statusColumn:
                return tr("Status");
            case customCostColumn:
                return tr("Custom cost");
            case priceColumn:
                return tr("Price");
            case priceStatusColumn:
                return tr("Price status");
            case volumeColumn:
                return tr("Volume");
            case totalColumn:
                return tr("Total");
            case deltaColumn:
                return tr("Delta");
            case profitColumn:
                return tr("Profit");
            case totalProfitColumn:
                return tr("Total profit");
            case profitPerItemColumn:
                return tr("Profit per item");
            case timeLeftColumn:
                return tr("Time left");
            case orderAgeColumn:
                return tr("Order age");
            case firstSeenColumn:
                return tr("First issued");
            case stationColumn:
                return tr("Station");
            }
        }

        return QVariant{};
    }

    QModelIndex MarketOrderSellModel::index(int row, int column, const QModelIndex &parent) const
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

    QModelIndex MarketOrderSellModel::parent(const QModelIndex &index) const
    {
        if (!index.isValid())
             return QModelIndex{};

         auto childItem = static_cast<const TreeItem *>(index.internalPointer());
         auto parentItem = childItem->parent();

         if (parentItem == &mRootItem)
             return QModelIndex{};

         return createIndex(parentItem->row(), 0, parentItem);
    }

    int MarketOrderSellModel::rowCount(const QModelIndex &parent) const
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

    size_t MarketOrderSellModel::getOrderCount() const
    {
        return mTotalOrders;
    }

    quint64 MarketOrderSellModel::getVolumeRemaining() const
    {
        return mVolumeRemaining;
    }

    quint64 MarketOrderSellModel::getVolumeEntered() const
    {
        return mVolumeEntered;
    }

    double MarketOrderSellModel::getTotalISK() const
    {
        return mTotalISK;
    }

    double MarketOrderSellModel::getTotalSize() const
    {
        return mTotalSize;
    }

    MarketOrderModel::Range MarketOrderSellModel::getOrderRange(const QModelIndex &index) const
    {
        Range range;
        range.mFrom = mData[index.row()].getFirstSeen();
        range.mTo = mData[index.row()].getLastSeen();

        return range;
    }

    MarketOrderModel::OrderInfo MarketOrderSellModel::getOrderInfo(const QModelIndex &index) const
    {
        QSettings settings;

        const auto &data = mData[index.row()];
        const auto price = mDataProvider.getTypeSellPrice(data.getTypeId(), data.getLocationId());
        const auto priceDelta = settings.value(PriceSettings::priceDeltaKey, PriceSettings::priceDeltaDefault).toDouble();

        OrderInfo info;
        info.mOrderPrice = data.getPrice();
        info.mMarketPrice = price.getValue();
        info.mTargetPrice = (info.mMarketPrice < info.mOrderPrice) ? (info.mMarketPrice - priceDelta) : (info.mOrderPrice);
        info.mOrderLocalTimestamp = mCacheTimerProvider.getLocalUpdateTimer(mCharacterId, TimerType::MarketOrders);
        info.mMarketLocalTimestamp = price.getUpdateTime();

        return info;
    }

    EveType::IdType MarketOrderSellModel::getOrderTypeId(const QModelIndex &index) const
    {
        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        const auto order = item->getOrder();
        return (order != nullptr) ? (order->getTypeId()) : (EveType::invalidId);
    }

    const MarketOrder *MarketOrderSellModel::getOrder(const QModelIndex &index) const
    {
        if (!index.isValid())
            return nullptr;

        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        return item->getOrder();
    }

    WalletTransactionsModel::EntryType MarketOrderSellModel::getOrderTypeFilter() const
    {
        return WalletTransactionsModel::EntryType::Sell;
    }

    bool MarketOrderSellModel::shouldShowPriceInfo(const QModelIndex &index) const
    {
        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        return index.column() == priceColumn && item->getOrder() != nullptr;
    }

    void MarketOrderSellModel::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        reset();
    }

    void MarketOrderSellModel::setGrouping(Grouping grouping)
    {
        mGrouping = grouping;
        reset();
    }

    void MarketOrderSellModel::reset()
    {
        beginResetModel();

        mData = mOrderProvider.getSellOrders(mCharacterId);
        mRootItem.clearChildren();

        mVolumeRemaining = 0;
        mVolumeEntered = 0;
        mTotalISK = 0.;
        mTotalSize = 0.;

        std::unordered_map<quintptr, TreeItem *> groupItems;

        for (const auto &order : mData)
        {
            auto item = std::make_unique<TreeItem>();
            item->setOrder(&order);

            if (mGrouping != Grouping::None)
            {
                const auto id = getGroupingId(order);
                auto it = groupItems.find(id);

                if (it == std::end(groupItems))
                {
                    auto item = std::make_unique<TreeItem>();
                    item->setGroupName(getGroupingData(order));

                    auto itemPtr = item.get();
                    mRootItem.appendChild(std::move(item));

                    it = groupItems.emplace(id, itemPtr).first;
                }

                it->second->appendChild(std::move(item));
            }
            else
            {
                mRootItem.appendChild(std::move(item));
            }

            if (order.getState() != MarketOrder::State::Active)
                continue;

            mVolumeRemaining += order.getVolumeRemaining();
            mVolumeEntered += order.getVolumeEntered();
            mTotalISK += order.getPrice() * order.getVolumeRemaining();
            mTotalSize += mDataProvider.getTypeVolume(order.getTypeId()) * order.getVolumeRemaining();

            ++mTotalOrders;
        }

        endResetModel();
    }

    quintptr MarketOrderSellModel::getGroupingId(const MarketOrder &order) const
    {
        switch (mGrouping) {
        case Grouping::Group:
            return mDataProvider.getTypeMarketGroupParentId(order.getTypeId());
        case Grouping::Station:
            return order.getLocationId();
        case Grouping::Type:
            return order.getTypeId();
        default:
            return 0;
        }
    }

    QString MarketOrderSellModel::getGroupingData(const MarketOrder &order) const
    {
        switch (mGrouping) {
        case Grouping::Group:
            return mDataProvider.getTypeMarketGroupParentName(order.getTypeId());
        case Grouping::Station:
            return mDataProvider.getLocationName(order.getLocationId());
        case Grouping::Type:
            return mDataProvider.getTypeName(order.getTypeId());
        default:
            return QString{};
        }
    }
}
