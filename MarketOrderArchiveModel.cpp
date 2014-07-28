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
#include <QLocale>
#include <QColor>
#include <QIcon>
#include <QFont>

#include "MarketOrderProvider.h"
#include "CacheTimerProvider.h"
#include "ItemCostProvider.h"
#include "EveDataProvider.h"
#include "IconUtils.h"
#include "ItemPrice.h"
#include "ItemCost.h"

#include "MarketOrderArchiveModel.h"

namespace Evernus
{
    MarketOrderArchiveModel::MarketOrderArchiveModel(const MarketOrderProvider &orderProvider,
                                                     const EveDataProvider &dataProvider,
                                                     const ItemCostProvider &itemCostProvider,
                                                     QObject *parent)
        : MarketOrderTreeModel{dataProvider, parent}
        , mOrderProvider{orderProvider}
        , mItemCostProvider{itemCostProvider}
    {
    }

    int MarketOrderArchiveModel::columnCount(const QModelIndex &parent) const
    {
        return 10;
    }

    QVariant MarketOrderArchiveModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant{};

        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        const auto column = index.column();

        if (mGrouping != Grouping::None)
        {
            if (item->parent() == &mRootItem)
            {
                if (role == Qt::UserRole || (role == Qt::DisplayRole && column == groupingColumn))
                    return item->getGroupName();

                return QVariant{};
            }
        }

        const auto data = item->getOrder();

        switch (role) {
        case Qt::UserRole:
            switch (column) {
            case lastSeenColumn:
                return data->getLastSeen();
            case typeColumn:
                return static_cast<int>(data->getType());
            case nameColumn:
                return mDataProvider.getTypeName(data->getTypeId());
            case groupColumn:
                return getGroupName(data->getTypeId());
            case statusColumn:
                return static_cast<int>(data->getState());
            case customCostColumn:
                return mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId())->getCost();
            case priceColumn:
                return data->getPrice();
            case volumeColumn:
                return QVariantList{} << data->getVolumeRemaining() << data->getVolumeEntered();
            case profitColumn:
                {
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                    return data->getVolumeRemaining() * (data->getPrice() - cost->getCost());
                }
            case stationColumn:
                return mDataProvider.getLocationName(data->getLocationId());
            }
            break;
        case Qt::DisplayRole:
            {
                const char * const stateNames[] = { "Active", "Closed", "Fulfilled", "Cancelled", "Pending", "Character Deleted" };

                QLocale locale;

                switch (column) {
                case lastSeenColumn:
                    return locale.toString(data->getLastSeen().toLocalTime());
                case typeColumn:
                    return (data->getType() == MarketOrder::Type::Buy) ? (tr("Buy")) : (tr("Sell"));
                case nameColumn:
                    return mDataProvider.getTypeName(data->getTypeId());
                case groupColumn:
                    return getGroupName(data->getTypeId());
                case statusColumn:
                    {
                        const auto prefix = (data->getDelta() != 0) ? ("*") : ("");

                        if (data->getState() == MarketOrder::State::Fulfilled && data->getVolumeRemaining() > 0)
                            return tr("Expired");

                        if ((data->getState() >= MarketOrder::State::Active && data->getState() <= MarketOrder::State::CharacterDeleted))
                            return prefix + tr(stateNames[static_cast<size_t>(data->getState())]);
                    }
                    break;
                case customCostColumn:
                    {
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                        if (!cost->isNew())
                            return locale.toCurrencyString(cost->getCost(), "ISK");
                    }
                    break;
                case priceColumn:
                    return locale.toCurrencyString(data->getPrice(), "ISK");
                case volumeColumn:
                    return QString{"%1/%2"}.arg(locale.toString(data->getVolumeRemaining())).arg(locale.toString(data->getVolumeEntered()));
                case stationColumn:
                    return mDataProvider.getLocationName(data->getLocationId());
                }
            }
            break;
        case Qt::FontRole:
            if (column == statusColumn && data->getDelta() != 0)
            {
                QFont font;
                font.setBold(true);

                return font;
            }
            break;
        case Qt::DecorationRole:
            if (column == nameColumn)
            {
                const auto metaIcon = IconUtils::getIconForMetaGroup(mDataProvider.getTypeMetaGroupName(data->getTypeId()));
                if (!metaIcon.isNull())
                    return metaIcon;
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
            case profitColumn:
                return QColor{Qt::darkGreen};
            }
        }

        return QVariant{};
    }

    QVariant MarketOrderArchiveModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case lastSeenColumn:
                return tr("Completed");
            case typeColumn:
                return tr("Type");
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
            case volumeColumn:
                return tr("Volume");
            case profitColumn:
                return tr("Profit");
            case stationColumn:
                return tr("Station");
            }
        }

        return QVariant{};
    }

    MarketOrderModel::OrderInfo MarketOrderArchiveModel::getOrderInfo(const QModelIndex &index) const
    {
        return OrderInfo{};
    }

    WalletTransactionsModel::EntryType MarketOrderArchiveModel::getOrderTypeFilter(const QModelIndex &index) const
    {
        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        const auto order = item->getOrder();
        if (order == nullptr)
            return WalletTransactionsModel::EntryType::All;

        return (order->getType() == MarketOrder::Type::Buy) ?
               (WalletTransactionsModel::EntryType::Buy) :
               (WalletTransactionsModel::EntryType::Sell);
    }

    bool MarketOrderArchiveModel::shouldShowPriceInfo(const QModelIndex &index) const
    {
        return false;
    }

    int MarketOrderArchiveModel::getVolumeColumn() const
    {
        return volumeColumn;
    }

    void MarketOrderArchiveModel::setCharacterAndRange(Character::IdType id, const QDateTime &from, const QDateTime &to)
    {
        mFrom = from;
        mTo = to;

        setCharacter(id);
    }

    MarketOrderTreeModel::OrderList MarketOrderArchiveModel::getOrders() const
    {
        return mOrderProvider.getArchivedOrders(mCharacterId, mFrom, mTo);
    }
}
