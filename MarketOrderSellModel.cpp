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

#include "MarketOrderProvider.h"
#include "ItemCostProvider.h"
#include "EveDataProvider.h"
#include "PriceSettings.h"
#include "ItemPrice.h"
#include "ItemCost.h"

#include "MarketOrderSellModel.h"

namespace Evernus
{
    MarketOrderSellModel::MarketOrderSellModel(const MarketOrderProvider &orderProvider,
                                               const EveDataProvider &dataProvider,
                                               const ItemCostProvider &itemCostProvider,
                                               QObject *parent)
        : MarketOrderModel{parent}
        , mOrderProvider{orderProvider}
        , mDataProvider{dataProvider}
        , mItemCostProvider{itemCostProvider}
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

        const auto column = index.column();
        const auto row = index.row();
        const auto &data = mData[row];

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
        case Qt::UserRole:
            switch (column) {
            case nameColumn:
                return mDataProvider.getTypeName(data.getTypeId());
            case groupColumn:
                return mDataProvider.getTypeMarketGroupName(data.getTypeId());
            case statusColumn:
                return static_cast<int>(data.getState());
            case customCostColumn:
                return mItemCostProvider.fetchForCharacterAndType(mCharacterId, data.getTypeId()).getCost();
            case priceColumn:
                return data.getPrice();
            case priceStatusColumn:
                {
                    const auto price = mDataProvider.getTypeSellPrice(data.getTypeId(), data.getLocationId());
                    if (price.isNew())
                        return 0;

                    QSettings settings;
                    const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                    if (price.getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                        return 1;

                    return 2;
                }
            case volumeColumn:
                return data.getVolumeRemaining();
            case totalColumn:
                return data.getVolumeRemaining() * data.getPrice();
            case deltaColumn:
                return data.getDelta();
            case profitColumn:
                {
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data.getTypeId());
                    return data.getVolumeRemaining() * (data.getPrice() - cost.getCost());
                }
            case totalProfitColumn:
                {
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data.getTypeId());
                    return data.getVolumeEntered() * (data.getPrice() - cost.getCost());
                }
            case profitPerItemColumn:
                {
                    const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data.getTypeId());
                    return data.getPrice() - cost.getCost();
                }
            case timeLeftColumn:
                {
                    const auto timeEnd = data.getIssued().addDays(data.getDuration()).toMSecsSinceEpoch() / 1000;
                    const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                    if (timeEnd > timeCur)
                        return timeEnd - timeCur;
                }
                break;
            case orderAgeColumn:
                {
                    const auto timeStart = data.getIssued().toMSecsSinceEpoch() / 1000;
                    const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                    if (timeCur > timeStart)
                        return timeCur - timeStart;
                }
                break;
            case firstSeenColumn:
                return data.getFirstSeen();
            case stationColumn:
                return mDataProvider.getLocationName(data.getLocationId());
            }
            break;
        case Qt::DisplayRole:
            {
                const char * const stateNames[] = { "Active", "Closed", "Fulfilled", "Cancelled", "Pending", "Character Deleted" };

                QLocale locale;

                switch (column) {
                case nameColumn:
                    return mDataProvider.getTypeName(data.getTypeId());
                case groupColumn:
                    return mDataProvider.getTypeMarketGroupName(data.getTypeId());
                case statusColumn:
                    {
                        const auto prefix = (data.getDelta() != 0) ? ("*") : ("");

                        if ((data.getState() >= MarketOrder::State::Active && data.getState() <= MarketOrder::State::CharacterDeleted))
                            return prefix + tr(stateNames[static_cast<size_t>(data.getState())]);

                        if (data.getState() == MarketOrder::State::Archived)
                            return tr("Archived");
                    }
                    break;
                case customCostColumn:
                    {
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data.getTypeId());
                        if (!cost.isNew())
                            return locale.toCurrencyString(cost.getCost(), "ISK");
                    }
                    break;
                case priceColumn:
                    return locale.toCurrencyString(data.getPrice(), "ISK");
                case priceStatusColumn:
                    {
                        const auto price = mDataProvider.getTypeSellPrice(data.getTypeId(), data.getLocationId());
                        if (price.isNew())
                            return tr("No price data");

                        QSettings settings;
                        const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                        if (price.getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                            return tr("Data too old");

                        return tr("OK");
                    }
                case volumeColumn:
                    return QString{"%1/%2"}.arg(locale.toString(data.getVolumeRemaining())).arg(locale.toString(data.getVolumeEntered()));
                case totalColumn:
                    return locale.toCurrencyString(data.getVolumeRemaining() * data.getPrice(), "ISK");
                case deltaColumn:
                    return locale.toString(data.getDelta());
                case profitColumn:
                    {
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data.getTypeId());
                        return locale.toCurrencyString(data.getVolumeRemaining() * (data.getPrice() - cost.getCost()), "ISK");
                    }
                case totalProfitColumn:
                    {
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data.getTypeId());
                        return locale.toCurrencyString(data.getVolumeEntered() * (data.getPrice() - cost.getCost()), "ISK");
                    }
                case profitPerItemColumn:
                    {
                        const auto cost = mItemCostProvider.fetchForCharacterAndType(mCharacterId, data.getTypeId());
                        return locale.toCurrencyString(data.getPrice() - cost.getCost(), "ISK");
                    }
                case timeLeftColumn:
                    {
                        const auto timeEnd = data.getIssued().addDays(data.getDuration()).toMSecsSinceEpoch() / 1000;
                        const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                        if (timeEnd > timeCur)
                            return secondsToString(timeEnd - timeCur);
                    }
                    break;
                case orderAgeColumn:
                    {
                        const auto timeStart = data.getIssued().toMSecsSinceEpoch() / 1000;
                        const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                        if (timeCur > timeStart)
                            return secondsToString(timeCur - timeStart);
                    }
                    break;
                case firstSeenColumn:
                    return locale.toString(data.getFirstSeen());
                case stationColumn:
                    return mDataProvider.getLocationName(data.getLocationId());
                }
            }
            break;
        case Qt::FontRole:
            if ((column == statusColumn && data.getDelta() != 0) || (column == statusColumn && data.getState() == MarketOrder::State::Fulfilled))
            {
                QFont font;
                font.setBold(true);

                return font;
            }
            break;
        case Qt::BackgroundRole:
            if (column == priceColumn)
            {
                const auto price = mDataProvider.getTypeSellPrice(data.getTypeId(), data.getLocationId());
                if (!price.isNew())
                {
                    if (price.getValue() > data.getPrice())
                        return QColor{255, 192, 192};

                    QSettings settings;
                    const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
                    if (price.getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge))
                        return QColor{255, 255, 192};
                }
            }
            break;
        case Qt::ForegroundRole:
            switch (column) {
            case statusColumn:
                switch (data.getState()) {
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
                    return QColor{0, 64, 0};
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
            if (column == volumeColumn)
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
        return (hasIndex(row, column, parent)) ? (createIndex(row, column)) : (QModelIndex{});
    }

    QModelIndex MarketOrderSellModel::parent(const QModelIndex &index) const
    {
        return QModelIndex{};
    }

    int MarketOrderSellModel::rowCount(const QModelIndex &parent) const
    {
        if (parent.isValid())
            return 0;

        return static_cast<int>(mData.size());
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

    void MarketOrderSellModel::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        reset();
    }

    void MarketOrderSellModel::reset()
    {
        beginResetModel();

        mData = mOrderProvider.getSellOrders(mCharacterId);

        mVolumeRemaining = 0;
        mVolumeEntered = 0;
        mTotalISK = 0.;
        mTotalSize = 0.;

        for (const auto &order : mData)
        {
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
}
