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
#include <QIcon>
#include <QFont>

#include "MarketOrderProvider.h"
#include "CharacterRepository.h"
#include "CacheTimerProvider.h"
#include "ItemCostProvider.h"
#include "EveDataProvider.h"
#include "PriceSettings.h"
#include "ExternalOrder.h"
#include "IconUtils.h"
#include "TextUtils.h"

#include "MarketOrderBuyModel.h"

namespace Evernus
{
    MarketOrderBuyModel::MarketOrderBuyModel(const MarketOrderProvider &orderProvider,
                                             const EveDataProvider &dataProvider,
                                             const CacheTimerProvider &cacheTimerProvider,
                                             const CharacterRepository &characterRepository,
                                             bool corp,
                                             QObject *parent)
        : MarketOrderTreeModel{dataProvider, parent}
        , mOrderProvider{orderProvider}
        , mCacheTimerProvider{cacheTimerProvider}
        , mCharacterRepository{characterRepository}
        , mCorp{corp}
    {
    }

    int MarketOrderBuyModel::columnCount(const QModelIndex &parent) const
    {
        return (mCorp) ? (15) : (14);
    }

    QVariant MarketOrderBuyModel::data(const QModelIndex &index, int role) const
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
        case Qt::ToolTipRole:
            if (column == priceColumn)
            {
                const auto price = mDataProvider.getTypeBuyPrice(data->getTypeId(), data->getLocationId());
                if (price->isNew())
                    return tr("No price data -> Please import prices from Orders/Assets tab or by using Margin tool.");

                QLocale locale;

                if (price->getValue() > data->getPrice())
                {
                    return tr("You have been overbid. Current price is %1 (%2 different from yours).\nClick the icon for details.")
                        .arg(locale.toCurrencyString(price->getValue(), "ISK"))
                        .arg(locale.toCurrencyString(price->getValue() - data->getPrice(), "ISK"));
                }

                QSettings settings;
                const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                if (price->getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                {
                    return tr("Price data is too old (valid on %1).\nPlease import prices from Orders/Assets tab or by using Margin tool.")
                        .arg(TextUtils::dateTimeToString(price->getUpdateTime().toLocalTime(), locale));
                }

                return tr("Your price was best on %1").arg(TextUtils::dateTimeToString(price->getUpdateTime().toLocalTime(), locale));
            }
            break;
        case Qt::DecorationRole:
            if (column == priceColumn)
            {
                const auto price = mDataProvider.getTypeBuyPrice(data->getTypeId(), data->getLocationId());
                if (price->isNew())
                    return QIcon{":/images/error.png"};

                if (price->getValue() > data->getPrice())
                    return QIcon{":/images/exclamation.png"};

                QSettings settings;
                const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                if (price->getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
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
                return getGroupName(data->getTypeId());
            case statusColumn:
                return static_cast<int>(data->getState());
            case priceColumn:
                return data->getPrice();
            case priceStatusColumn:
                {
                    const auto price = mDataProvider.getTypeBuyPrice(data->getTypeId(), data->getLocationId());
                    if (price->isNew())
                        return static_cast<int>(PriceStatus::NoData);

                    QSettings settings;
                    const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                    if (price->getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                        return static_cast<int>(PriceStatus::DataTooOld);

                    return static_cast<int>(PriceStatus::Ok);
                }
            case volumeColumn:
                return QVariantList{} << data->getVolumeRemaining() << data->getVolumeEntered();
            case totalColumn:
                return data->getVolumeRemaining() * data->getPrice();
            case deltaColumn:
                return data->getDelta();
            case rangeColumn:
                return data->getRange();
            case minQuantityColumn:
                return data->getMinVolume();
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
            case ownerColumn:
                try
                {
                    return mCharacterRepository.getCharacterName(data->getCharacterId());
                }
                catch (const CharacterRepository::NotFoundException &)
                {
                }
            }
            break;
        case Qt::DisplayRole:
            {
                const char * const stateNames[] = {
                    QT_TR_NOOP("Active"),
                    QT_TR_NOOP("Closed"),
                    QT_TR_NOOP("Fulfilled"),
                    QT_TR_NOOP("Cancelled"),
                    QT_TR_NOOP("Pending"),
                    QT_TR_NOOP("Character Deleted")
                };

                QLocale locale;

                switch (column) {
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
                case priceColumn:
                    return locale.toCurrencyString(data->getPrice(), "ISK");
                case priceStatusColumn:
                    {
                        const auto price = mDataProvider.getTypeBuyPrice(data->getTypeId(), data->getLocationId());
                        if (price->isNew())
                            return tr("No price data");

                        QSettings settings;
                        const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                        if (price->getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
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
                case rangeColumn:
                    {
                        const auto range = data->getRange();
                        switch (range) {
                        case -1:
                            return tr("Station");
                        case 0:
                            return tr("System");
                        case 32767:
                            return tr("Region");
                        default:
                            return tr("%1 jumps").arg(range);
                        }
                    }
                case minQuantityColumn:
                    return locale.toString(data->getMinVolume());
                case timeLeftColumn:
                    {
                        const auto timeEnd = data->getIssued().addDays(data->getDuration()).toMSecsSinceEpoch() / 1000;
                        const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                        if (timeEnd > timeCur)
                            return TextUtils::secondsToString(timeEnd - timeCur);
                    }
                    break;
                case orderAgeColumn:
                    {
                        const auto timeStart = data->getIssued().toMSecsSinceEpoch() / 1000;
                        const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                        if (timeCur > timeStart)
                            return TextUtils::secondsToString(timeCur - timeStart);
                    }
                    break;
                case firstSeenColumn:
                    return TextUtils::dateTimeToString(data->getFirstSeen().toLocalTime(), locale);
                case stationColumn:
                    return mDataProvider.getLocationName(data->getLocationId());
                case ownerColumn:
                    try
                    {
                        return mCharacterRepository.getCharacterName(data->getCharacterId());
                    }
                    catch (const CharacterRepository::NotFoundException &)
                    {
                    }
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
        case Qt::BackgroundRole:
            if (column == priceColumn)
            {
                const auto price = mDataProvider.getTypeBuyPrice(data->getTypeId(), data->getLocationId());
                if (!price->isNew())
                {
                    if (price->getValue() > data->getPrice())
                        return QColor{255, 192, 192};

                    QSettings settings;
                    const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                    if (price->getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
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

    QVariant MarketOrderBuyModel::headerData(int section, Qt::Orientation orientation, int role) const
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
            case rangeColumn:
                return tr("Range");
            case minQuantityColumn:
                return tr("Min. quantity");
            case timeLeftColumn:
                return tr("Time left");
            case orderAgeColumn:
                return tr("Order age");
            case firstSeenColumn:
                return tr("First issued");
            case stationColumn:
                return tr("Station");
            case ownerColumn:
                return tr("Owner");
            }
        }

        return QVariant{};
    }

    MarketOrderModel::OrderInfo MarketOrderBuyModel::getOrderInfo(const QModelIndex &index) const
    {
        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        const auto order = item->getOrder();
        if (order == nullptr)
            return OrderInfo{};

        QSettings settings;

        const auto price = mDataProvider.getTypeBuyPrice(order->getTypeId(), order->getLocationId());
        const auto priceDelta = settings.value(PriceSettings::priceDeltaKey, PriceSettings::priceDeltaDefault).toDouble();

        OrderInfo info;
        info.mOrderPrice = order->getPrice();
        info.mMarketPrice = price->getValue();
        info.mTargetPrice = (info.mMarketPrice > info.mOrderPrice) ? (info.mMarketPrice + priceDelta) : (info.mOrderPrice);
        info.mOrderLocalTimestamp = mCacheTimerProvider.getLocalUpdateTimer(mCharacterId, TimerType::MarketOrders);
        info.mMarketLocalTimestamp = price->getUpdateTime().toLocalTime();

        return info;
    }

    WalletTransactionsModel::EntryType MarketOrderBuyModel::getOrderTypeFilter(const QModelIndex &index) const
    {
        Q_UNUSED(index);
        return WalletTransactionsModel::EntryType::Buy;
    }

    bool MarketOrderBuyModel::shouldShowPriceInfo(const QModelIndex &index) const
    {
        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        return index.column() == priceColumn && item->getOrder() != nullptr;
    }

    int MarketOrderBuyModel::getVolumeColumn() const
    {
        return volumeColumn;
    }

    MarketOrderTreeModel::OrderList MarketOrderBuyModel::getOrders() const
    {
        try
        {
            return (mCorp) ?
                   (mOrderProvider.getBuyOrdersForCorporation(mCharacterRepository.getCorporationId(mCharacterId))) :
                   (mOrderProvider.getBuyOrders(mCharacterId));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            return OrderList{};
        }
    }
}
