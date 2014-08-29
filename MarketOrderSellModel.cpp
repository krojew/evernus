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
#include "PriceUtils.h"
#include "IconUtils.h"
#include "TextUtils.h"

#include "MarketOrderSellModel.h"

namespace Evernus
{
    MarketOrderSellModel::MarketOrderSellModel(const MarketOrderProvider &orderProvider,
                                               const EveDataProvider &dataProvider,
                                               const ItemCostProvider &itemCostProvider,
                                               const CacheTimerProvider &cacheTimerProvider,
                                               const CharacterRepository &characterRepository,
                                               bool corp,
                                               QObject *parent)
        : MarketOrderTreeModel{dataProvider, parent}
        , mOrderProvider{orderProvider}
        , mItemCostProvider{itemCostProvider}
        , mCacheTimerProvider{cacheTimerProvider}
        , mCharacterRepository{characterRepository}
        , mCorp{corp}
    {
        if (mCorp)
            connect(&mDataProvider, &EveDataProvider::namesChanged, this, &MarketOrderSellModel::updateNames);
    }

    int MarketOrderSellModel::columnCount(const QModelIndex &parent) const
    {
        return (mCorp) ? (numColumns) : (numColumns - 1);
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
                if (role == Qt::UserRole || (role == Qt::DisplayRole && column == groupingColumn))
                    return item->getGroupName();

                return QVariant{};
            }
        }

        Q_ASSERT(mCharacter);

        const auto data = item->getOrder();

        switch (role) {
        case Qt::ToolTipRole:
            if (column == priceColumn)
            {
                const auto price = mDataProvider.getTypeSellPrice(data->getTypeId(), data->getStationId());
                if (price->isNew())
                    return tr("No price data -> Please import prices from Orders/Assets tab or by using Margin tool.");

                QLocale locale;

                if (price->getPrice() < data->getPrice())
                {
                    return tr("You have been undercut. Current price is %1 (%2 different from yours).\nClick the icon for details.")
                        .arg(locale.toCurrencyString(price->getPrice(), "ISK"))
                        .arg(locale.toCurrencyString(price->getPrice() - data->getPrice(), "ISK"));
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
                const auto price = mDataProvider.getTypeSellPrice(data->getTypeId(), data->getStationId());
                if (price->isNew())
                    return QIcon{":/images/error.png"};

                if (price->getPrice() < data->getPrice())
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
            case customCostColumn:
                return mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId())->getCost();
            case priceColumn:
                return data->getPrice();
            case priceStatusColumn:
                {
                    const auto price = mDataProvider.getTypeSellPrice(data->getTypeId(), data->getStationId());
                    if (price->isNew())
                        return 1;

                    QSettings settings;
                    const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                    if (price->getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                        return 2;

                    return (price->getPrice() < data->getPrice()) ? (0) : (3);
                }
            case priceDifferenceColumn:
                return mDataProvider.getTypeSellPrice(data->getTypeId(), data->getStationId())->getPrice() - data->getPrice();
            case volumeColumn:
                return QVariantList{} << data->getVolumeRemaining() << data->getVolumeEntered();
            case totalColumn:
                return data->getVolumeRemaining() * data->getPrice();
            case deltaColumn:
                return data->getDelta();
            case marginColumn:
                {
                    const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                    return PriceUtils::getMargin(cost->getCost(), data->getPrice(), taxes);
                }
            case profitColumn:
                {
                    const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                    const auto realCost = PriceUtils::getCoS(cost->getCost(), taxes);
                    const auto realPrice = PriceUtils::getRevenue(data->getPrice(), taxes);
                    return data->getVolumeRemaining() * (realPrice - realCost);
                }
            case totalProfitColumn:
                {
                    const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                    const auto realCost = PriceUtils::getCoS(cost->getCost(), taxes);
                    const auto realPrice = PriceUtils::getRevenue(data->getPrice(), taxes);
                    return data->getVolumeEntered() * (realPrice - realCost);
                }
            case profitPerItemColumn:
                {
                    const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                    const auto realCost = PriceUtils::getCoS(cost->getCost(), taxes);
                    const auto realPrice = PriceUtils::getRevenue(data->getPrice(), taxes);
                    return realPrice - realCost;
                }
            case etaColumn:
                {
                    const auto elapsed = data->getIssued().daysTo(QDateTime::currentDateTimeUtc());
                    if (elapsed <= 0)
                        return 0;

                    const auto movement = (data->getVolumeEntered() - data->getVolumeRemaining()) / elapsed;
                    return (movement > 0) ? (data->getVolumeRemaining() / movement) : (0);
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
                return mDataProvider.getLocationName(data->getStationId());
            case ownerColumn:
                return getCharacterName(data->getCharacterId());
            }
            break;
        case Qt::DisplayRole:
            {
                QLocale locale;

                switch (column) {
                case nameColumn:
                    return mDataProvider.getTypeName(data->getTypeId());
                case groupColumn:
                    return getGroupName(data->getTypeId());
                case statusColumn:
                    {
                        const char * const stateNames[] = {
                            QT_TR_NOOP("Active"),
                            QT_TR_NOOP("Closed"),
                            QT_TR_NOOP("Fulfilled"),
                            QT_TR_NOOP("Cancelled"),
                            QT_TR_NOOP("Pending"),
                            QT_TR_NOOP("Character Deleted")
                        };

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
                case priceStatusColumn:
                    {
                        const auto price = mDataProvider.getTypeSellPrice(data->getTypeId(), data->getStationId());
                        if (price->isNew())
                            return tr("No price data");

                        QSettings settings;
                        const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                        if (price->getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                            return tr("Data too old");
                    }
                    break;
                case priceDifferenceColumn:
                    return locale.toCurrencyString(mDataProvider.getTypeSellPrice(data->getTypeId(), data->getStationId())->getPrice() - data->getPrice(), "ISK");
                case volumeColumn:
                    return QString{"%1/%2"}.arg(locale.toString(data->getVolumeRemaining())).arg(locale.toString(data->getVolumeEntered()));
                case totalColumn:
                    return locale.toCurrencyString(data->getVolumeRemaining() * data->getPrice(), "ISK");
                case deltaColumn:
                    if (data->getDelta() != 0)
                        return locale.toString(data->getDelta());
                    break;
                case marginColumn:
                    {
                        const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                        return QString{"%1%2"}.arg(locale.toString(PriceUtils::getMargin(cost->getCost(), data->getPrice(), taxes), 'f', 2)).arg(locale.percent());
                    }
                case profitColumn:
                    {
                        const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                        const auto realCost = PriceUtils::getCoS(cost->getCost(), taxes);
                        const auto realPrice = PriceUtils::getRevenue(data->getPrice(), taxes);
                        return locale.toCurrencyString(data->getVolumeRemaining() * (realPrice - realCost), "ISK");
                    }
                case totalProfitColumn:
                    {
                        const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                        const auto realCost = PriceUtils::getCoS(cost->getCost(), taxes);
                        const auto realPrice = PriceUtils::getRevenue(data->getPrice(), taxes);
                        return locale.toCurrencyString(data->getVolumeEntered() * (realPrice - realCost), "ISK");
                    }
                case profitPerItemColumn:
                    {
                        const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                        const auto realCost = PriceUtils::getCoS(cost->getCost(), taxes);
                        const auto realPrice = PriceUtils::getRevenue(data->getPrice(), taxes);
                        return locale.toCurrencyString(realPrice - realCost, "ISK");
                    }
                case etaColumn:
                    {
                        const auto elapsed = data->getIssued().daysTo(QDateTime::currentDateTimeUtc());
                        if (elapsed <= 0)
                            return tr("unknown");

                        const auto movement = (data->getVolumeEntered() - data->getVolumeRemaining()) / elapsed;
                        if (movement <= 0)
                            return tr("unknown");

                        const auto eta = data->getVolumeRemaining() / movement;
                        return (eta > 0) ? (tr("%n day(s)", "", eta)) : (tr("today"));
                    }
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
                    return mDataProvider.getLocationName(data->getStationId());
                case ownerColumn:
                    return getCharacterName(data->getCharacterId());
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
                const auto price = mDataProvider.getTypeSellPrice(data->getTypeId(), data->getStationId());
                if (!price->isNew())
                {
                    if (price->getPrice() < data->getPrice())
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
            case customCostColumn:
            case priceStatusColumn:
                return QColor{Qt::darkRed};
            case marginColumn:
                {
                    const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data->getTypeId());
                    const auto realCost = PriceUtils::getCoS(cost->getCost(), taxes);
                    const auto realPrice = PriceUtils::getRevenue(data->getPrice(), taxes);
                    const auto margin = 100. * (realPrice - realCost) / realPrice;

                    QSettings settings;
                    if (margin < settings.value(PriceSettings::minMarginKey, PriceSettings::minMarginDefault).toDouble())
                        return QColor{Qt::red};
                    if (margin < settings.value(PriceSettings::preferredMarginKey, PriceSettings::preferredMarginDefault).toDouble())
                        return QColor{0xff, 0xa5, 0x00};

                    return QColor{Qt::green};
                }
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
            case priceDifferenceColumn:
                return tr("Price difference");
            case volumeColumn:
                return tr("Volume");
            case totalColumn:
                return tr("Total");
            case deltaColumn:
                return tr("Delta");
            case marginColumn:
                return tr("Margin");
            case profitColumn:
                return tr("Profit");
            case totalProfitColumn:
                return tr("Total profit");
            case profitPerItemColumn:
                return tr("Profit per item");
            case etaColumn:
                return tr("ETA");
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

    MarketOrderModel::OrderInfo MarketOrderSellModel::getOrderInfo(const QModelIndex &index) const
    {
        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        const auto order = item->getOrder();
        if (order == nullptr)
            return OrderInfo{};

        QSettings settings;

        const auto price = mDataProvider.getTypeSellPrice(order->getTypeId(), order->getStationId());
        const auto priceDelta = settings.value(PriceSettings::priceDeltaKey, PriceSettings::priceDeltaDefault).toDouble();

        OrderInfo info;
        info.mOrderPrice = order->getPrice();
        info.mMarketPrice = price->getPrice();
        info.mTargetPrice = (info.mMarketPrice < info.mOrderPrice) ? (info.mMarketPrice - priceDelta) : (info.mOrderPrice);
        info.mOrderLocalTimestamp = mCacheTimerProvider.getLocalUpdateTimer(mCharacterId, TimerType::MarketOrders);
        info.mMarketLocalTimestamp = price->getUpdateTime().toLocalTime();

        return info;
    }

    WalletTransactionsModel::EntryType MarketOrderSellModel::getOrderTypeFilter(const QModelIndex &index) const
    {
        Q_UNUSED(index);
        return WalletTransactionsModel::EntryType::Sell;
    }

    bool MarketOrderSellModel::shouldShowPriceInfo(const QModelIndex &index) const
    {
        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        return index.column() == priceColumn && item->getOrder() != nullptr;
    }

    int MarketOrderSellModel::getVolumeColumn() const
    {
        return volumeColumn;
    }

    MarketOrderModel::Type MarketOrderSellModel::getType() const
    {
        return Type::Sell;
    }

    void MarketOrderSellModel::updateNames()
    {
        if (mGrouping == Grouping::None)
        {
            emit dataChanged(index(0, ownerColumn),
                             index(rowCount() - 1, ownerColumn),
                             QVector<int>{}
                                 << Qt::UserRole
                                 << Qt::DisplayRole);
        }
        else
        {
            const auto rows = mRootItem.childCount();
            for (auto row = 0; row < rows; ++row)
            {
                const auto child = mRootItem.child(row);
                const auto children = child->childCount();
                const auto parent = index(row, 0);

                emit dataChanged(index(0, ownerColumn, parent),
                                 index(children - 1, ownerColumn, parent),
                                 QVector<int>{}
                                     << Qt::UserRole
                                     << Qt::DisplayRole);
            }
        }
    }

    MarketOrderTreeModel::OrderList MarketOrderSellModel::getOrders() const
    {
        if (!mCharacter)
            return OrderList{};

        return (mCorp) ?
               (mOrderProvider.getSellOrdersForCorporation(mCharacter->getCorporationId())) :
               (mOrderProvider.getSellOrders(mCharacterId));
    }

    void MarketOrderSellModel::handleNewCharacter()
    {
        try
        {
            mCharacter = mCharacterRepository.find(mCharacterId);
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            mCharacter.reset();
        }
    }

    QString MarketOrderSellModel::getCharacterName(Character::IdType id) const
    {
        return mDataProvider.getGenericName(id);
    }
}
