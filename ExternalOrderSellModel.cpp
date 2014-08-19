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
#include <algorithm>
#include <limits>

#include <QLocale>
#include <QColor>

#include "ExternalOrderRepository.h"
#include "CharacterRepository.h"
#include "MarketOrderProvider.h"
#include "ItemCostProvider.h"
#include "EveDataProvider.h"
#include "TextUtils.h"

#include "ExternalOrderSellModel.h"

namespace Evernus
{
    ExternalOrderSellModel::ExternalOrderSellModel(const EveDataProvider &dataProvider,
                                                   const ExternalOrderRepository &orderRepo,
                                                   const CharacterRepository &characterRepo,
                                                   const MarketOrderProvider &orderProvider,
                                                   const MarketOrderProvider &corpOrderProvider,
                                                   const ItemCostProvider &costProvider,
                                                   QObject *parent)
        : ExternalOrderModel{parent}
        , mDataProvider{dataProvider}
        , mOrderRepo{orderRepo}
        , mCharacterRepo{characterRepo}
        , mOrderProvider{orderProvider}
        , mCorpOrderProvider{corpOrderProvider}
        , mCostProvider{costProvider}
    {
    }

    int ExternalOrderSellModel::columnCount(const QModelIndex &parent) const
    {
        return 9;
    }

    QVariant ExternalOrderSellModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant{};

        const auto column = index.column();
        const auto &order = mOrders[index.row()];

        switch (role) {
        case Qt::DisplayRole:
            {
                QLocale locale;

                switch (column) {
                case stationColumn:
                    return mDataProvider.getLocationName(order->getStationId());
                case deviationColumn:
                    return QString{"%1%2"}.arg(static_cast<int>(computeDeviation(*order) * 100.)).arg(locale.percent());
                case priceColumn:
                    return locale.toCurrencyString(order->getPrice(), "ISK");
                case volumeColumn:
                    return QString{"%1/%2"}.arg(locale.toString(order->getVolumeRemaining())).arg(locale.toString(order->getVolumeEntered()));
                case totalProfitColumn:
                    return locale.toCurrencyString(order->getVolumeRemaining() * order->getPrice(), "ISK");
                case totalSizeColumn:
                    return QString{"%1m³"}.arg(locale.toString(order->getVolumeRemaining() * mDataProvider.getTypeVolume(order->getTypeId()), 'f', 2));
                case issuedColumn:
                    return TextUtils::dateTimeToString(order->getIssued().toLocalTime(), locale);
                case durationColumn:
                    {
                        const auto timeEnd = order->getIssued().addDays(order->getDuration()).toMSecsSinceEpoch() / 1000;
                        const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                        if (timeEnd > timeCur)
                            return TextUtils::secondsToString(timeEnd - timeCur);
                    }
                    break;
                case updatedColumn:
                    return TextUtils::dateTimeToString(order->getUpdateTime().toLocalTime(), locale);
                }
            }
            break;
        case Qt::UserRole:
            switch (column) {
            case stationColumn:
                return mDataProvider.getLocationName(order->getStationId());
            case deviationColumn:
                return computeDeviation(*order);
            case priceColumn:
                return order->getPrice();
            case volumeColumn:
                return QVariantList{} << order->getVolumeRemaining() << order->getVolumeEntered();
            case totalProfitColumn:
                return order->getVolumeRemaining() * order->getPrice();
            case totalSizeColumn:
                return order->getVolumeRemaining() * mDataProvider.getTypeVolume(order->getTypeId());
            case issuedColumn:
                return order->getIssued();
            case durationColumn:
                {
                    const auto timeEnd = order->getIssued().addDays(order->getDuration()).toMSecsSinceEpoch() / 1000;
                    const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                    if (timeEnd > timeCur)
                        return timeEnd - timeCur;
                }
                break;
            case updatedColumn:
                return order->getUpdateTime();
            }
            break;
        case Qt::ForegroundRole:
            if (column == totalProfitColumn)
                return QColor{Qt::darkGreen};
            break;
        case Qt::BackgroundRole:
            if (mOwnOrders.find(order->getId()) != std::end(mOwnOrders))
                return QColor{255, 255, 128};
            break;
        case Qt::ToolTipRole:
            if (mOwnOrders.find(order->getId()) != std::end(mOwnOrders))
                return tr("Your order");
            break;
        case Qt::TextAlignmentRole:
            if (column == volumeColumn)
                return Qt::AlignRight;
        }

        return QVariant{};
    }

    QVariant ExternalOrderSellModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case stationColumn:
                return tr("Station");
            case deviationColumn:
                return tr("Deviation");
            case priceColumn:
                return tr("Price");
            case volumeColumn:
                return tr("Volume");
            case totalProfitColumn:
                return tr("Total profit");
            case totalSizeColumn:
                return tr("Total size");
            case issuedColumn:
                return tr("Issued");
            case durationColumn:
                return tr("Time left");
            case updatedColumn:
                return tr("Imported");
            }
        }

        return QVariant{};
    }

    QModelIndex ExternalOrderSellModel::index(int row, int column, const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return createIndex(row, column);

        return QModelIndex{};
    }

    QModelIndex ExternalOrderSellModel::parent(const QModelIndex &index) const
    {
        return QModelIndex{};
    }

    int ExternalOrderSellModel::rowCount(const QModelIndex &parent) const
    {
        return (parent.isValid()) ? (0) : (static_cast<int>(mOrders.size()));
    }

    int ExternalOrderSellModel::getPriceColumn() const
    {
        return priceColumn;
    }

    Qt::SortOrder ExternalOrderSellModel::getPriceSortOrder() const
    {
        return Qt::AscendingOrder;
    }

    int ExternalOrderSellModel::getVolumeColumn() const
    {
        return volumeColumn;
    }

    uint ExternalOrderSellModel::getTotalVolume() const
    {
        return mTotalVolume;
    }

    double ExternalOrderSellModel::getTotalSize() const
    {
        return mTotalSize;
    }

    double ExternalOrderSellModel::getTotalPrice() const
    {
        return mTotalPrice;
    }

    double ExternalOrderSellModel::getMedianPrice() const
    {
        return mMedianPrice;
    }

    double ExternalOrderSellModel::getMaxPrice() const
    {
        return mMaxPrice;
    }

    double ExternalOrderSellModel::getMinPrice() const
    {
        return mMinPrice;
    }

    void ExternalOrderSellModel::setCharacter(Character::IdType id)
    {
        beginResetModel();

        mCharacterId = id;
        mOwnOrders.clear();

        const auto orders = mOrderProvider.getSellOrders(mCharacterId);
        for (const auto &order : orders)
            mOwnOrders.emplace(order->getId());

        try
        {
            const auto character = mCharacterRepo.find(mCharacterId);
            const auto corpOrders = mCorpOrderProvider.getSellOrdersForCorporation(character->getCorporationId());
            for (const auto &order : corpOrders)
                mOwnOrders.emplace(order->getId());
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }

        endResetModel();
    }

    void ExternalOrderSellModel::setRegionId(uint id)
    {
        mRegionId = id;
    }

    void ExternalOrderSellModel::setSolarSystemId(uint id)
    {
        mSolarSystemId = id;
    }

    void ExternalOrderSellModel::setStationId(uint id)
    {
        mStationId = id;
    }

    EveType::IdType ExternalOrderSellModel::getTypeId() const noexcept
    {
        return mTypeId;
    }

    void ExternalOrderSellModel::setTypeId(EveType::IdType id) noexcept
    {
        mTypeId = id;
    }

    void ExternalOrderSellModel::reset()
    {
        beginResetModel();

        mMinPrice = std::numeric_limits<double>::max();
        mMedianPrice = mTotalPrice = mMaxPrice = mTotalSize = 0.;
        mTotalVolume = 0;

        if (mStationId != 0)
            mOrders = mOrderRepo.fetchSellByTypeAndStation(mTypeId, mStationId);
        else if (mSolarSystemId != 0)
            mOrders = mOrderRepo.fetchSellByTypeAndSolarSystem(mTypeId, mSolarSystemId);
        else if (mRegionId != 0)
            mOrders = mOrderRepo.fetchSellByTypeAndRegion(mTypeId, mRegionId);
        else
            mOrders = mOrderRepo.fetchSellByType(mTypeId);

        std::vector<double> prices;
        prices.reserve(mOrders.size());

        for (const auto &order : mOrders)
        {
            const auto price = order->getPrice();
            if (price < mMinPrice)
                mMinPrice = price;
            if (price > mMaxPrice)
                mMaxPrice = price;

            prices.emplace_back(price);

            const auto volume = order->getVolumeRemaining();

            mTotalPrice += price;
            mTotalSize += mDataProvider.getTypeVolume(order->getTypeId()) * volume;
            mTotalVolume += volume;
        }

        if (!prices.empty())
        {
            std::nth_element(std::begin(prices), std::next(std::begin(prices), prices.size() / 2), std::end(prices));
            mMedianPrice = prices[prices.size() / 2];
        }

        if (mMinPrice == std::numeric_limits<double>::max())
            mMinPrice = 0.;

        endResetModel();
    }

    void ExternalOrderSellModel::changeDeviationSource(DeviationSourceType type, double value)
    {
        beginResetModel();

        mDeviationType = type;
        mDeviationValue = value;

        endResetModel();
    }

    double ExternalOrderSellModel::computeDeviation(const ExternalOrder &order) const
    {
        switch (mDeviationType) {
        case DeviationSourceType::Median:
            return (mMedianPrice == 0.) ? (0.) : ((order.getPrice() - mMedianPrice) / mMedianPrice);
        case DeviationSourceType::Best:
            return (mMinPrice == 0.) ? (0.) : ((order.getPrice() - mMinPrice) / mMinPrice);
        case DeviationSourceType::Cost:
            {
                const auto cost = mCostProvider.fetchForCharacterAndType(mCharacterId, order.getTypeId())->getCost();
                return (cost == 0.) ? (0.) : ((order.getPrice() - cost) / cost);
            }
        case DeviationSourceType::Fixed:
            return (mDeviationValue == 0.) ? (0.) : ((order.getPrice() - mDeviationValue) / mDeviationValue);
        default:
            return 0.;
        }
    }
}
