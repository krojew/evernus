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
#include <algorithm>

#include <QLocale>
#include <QColor>

#include "ExternalOrderRepository.h"
#include "CharacterRepository.h"
#include "MarketOrderProvider.h"
#include "ItemCostProvider.h"
#include "EveDataProvider.h"
#include "TextUtils.h"

#include "ExternalOrderBuyModel.h"

namespace Evernus
{
    ExternalOrderBuyModel::ExternalOrderBuyModel(const EveDataProvider &dataProvider,
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

    int ExternalOrderBuyModel::columnCount(const QModelIndex &parent) const
    {
        return (mGrouping == Grouping::None) ? (11) : (8);
    }

    QVariant ExternalOrderBuyModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant{};

        const auto column = index.column();

        switch (mGrouping) {
        case Grouping::None:
            return getUngroupedData(column, role, *mOrders[index.row()]);
        case Grouping::Station:
            return getStationGroupedData(column, role, mGroupedData[index.row()]);
        case Grouping::System:
            return getSystemGroupedData(column, role, mGroupedData[index.row()]);
        case Grouping::Region:
            return getRegionGroupedData(column, role, mGroupedData[index.row()]);
        }

        return QVariant{};
    }

    QVariant ExternalOrderBuyModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            if (mGrouping == Grouping::None)
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
                case rangeColumn:
                    return tr("Range");
                case minQunatityColumn:
                    return tr("Min. quantity");
                case totalCostColumn:
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
            else
            {
                switch (section) {
                case groupByColumn:
                    switch (mGrouping) {
                    case Grouping::Station:
                        return tr("Station");
                    case Grouping::System:
                        return tr("Solar system");
                    case Grouping::Region:
                        return tr("Region");
                    default:
                        break;
                    }
                    break;
                case lowestPriceColumn:
                    return tr("Lowest price");
                case medianPriceColumn:
                    return tr("Median price");
                case highestPriceColumn:
                    return tr("Highest price");
                case volumeColumn:
                    return tr("Volume");
                case groupedTotalCostColumn:
                    return tr("Total profit");
                case ordersColumn:
                    return tr("Orders");
                case groupedTotalSizeColumn:
                    return tr("Total size");
                }
            }
        }

        return QVariant{};
    }

    QModelIndex ExternalOrderBuyModel::index(int row, int column, const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return createIndex(row, column);

        return QModelIndex{};
    }

    QModelIndex ExternalOrderBuyModel::parent(const QModelIndex &index) const
    {
        return QModelIndex{};
    }

    int ExternalOrderBuyModel::rowCount(const QModelIndex &parent) const
    {
        if (parent.isValid())
            return 0;

        return (mGrouping == Grouping::None) ? (static_cast<int>(mOrders.size())) : (static_cast<int>(mGroupedData.size()));
    }

    int ExternalOrderBuyModel::getPriceColumn() const
    {
        return (mGrouping == Grouping::None) ? (priceColumn) : (highestPriceColumn);
    }

    Qt::SortOrder ExternalOrderBuyModel::getPriceSortOrder() const
    {
        return Qt::DescendingOrder;
    }

    int ExternalOrderBuyModel::getVolumeColumn() const
    {
        return volumeColumn;
    }

    uint ExternalOrderBuyModel::getTotalVolume() const
    {
        return mTotalVolume;
    }

    double ExternalOrderBuyModel::getTotalSize() const
    {
        return mTotalSize;
    }

    double ExternalOrderBuyModel::getTotalPrice() const
    {
        return mTotalPrice;
    }

    double ExternalOrderBuyModel::getMedianPrice() const
    {
        return mMedianPrice;
    }

    double ExternalOrderBuyModel::getMaxPrice() const
    {
        return mMaxPrice;
    }

    double ExternalOrderBuyModel::getMinPrice() const
    {
        return mMinPrice;
    }

    const ExternalOrder &ExternalOrderBuyModel::getOrder(size_t row) const
    {
        return *mOrders[row];
    }

    double ExternalOrderBuyModel::getPrice(const QModelIndex &index) const
    {
        if (!index.isValid())
            return 0.;

        switch (mGrouping) {
        case Grouping::None:
            return mOrders[index.row()]->getPrice();
        case Grouping::Station:
        case Grouping::System:
        case Grouping::Region:
            return mGroupedData[index.row()].mLowestPrice;
        }

        return 0.;
    }

    void ExternalOrderBuyModel::setCharacter(Character::IdType id)
    {
        beginResetModel();

        mCharacterId = id;
        mOwnOrders.clear();

        const auto orders = mOrderProvider.getBuyOrders(mCharacterId);
        for (const auto &order : orders)
            mOwnOrders.emplace(order->getId());

        try
        {
            const auto character = mCharacterRepo.find(mCharacterId);
            const auto corpOrders = mCorpOrderProvider.getBuyOrdersForCorporation(character->getCorporationId());
            for (const auto &order : corpOrders)
                mOwnOrders.emplace(order->getId());
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }

        endResetModel();
    }

    void ExternalOrderBuyModel::setRegionId(uint id)
    {
        mRegionId = id;
    }

    void ExternalOrderBuyModel::setSolarSystemId(uint id)
    {
        mSolarSystemId = id;
    }

    void ExternalOrderBuyModel::setStationId(uint id)
    {
        mStationId = id;
    }

    EveType::IdType ExternalOrderBuyModel::getTypeId() const noexcept
    {
        return mTypeId;
    }

    void ExternalOrderBuyModel::setTypeId(EveType::IdType id) noexcept
    {
        mTypeId = id;
    }

    void ExternalOrderBuyModel::reset()
    {
        beginResetModel();

        mMinPrice = std::numeric_limits<double>::max();
        mMedianPrice = mTotalPrice = mMaxPrice = mTotalSize = 0.;
        mTotalVolume = 0;

        if (mStationId != 0)
            mOrders = mOrderRepo.fetchBuyByTypeAndStation(mTypeId, mStationId);
        else if (mSolarSystemId != 0)
            mOrders = mOrderRepo.fetchBuyByTypeAndSolarSystem(mTypeId, mSolarSystemId);
        else if (mRegionId != 0)
            mOrders = mOrderRepo.fetchBuyByTypeAndRegion(mTypeId, mRegionId);
        else
            mOrders = mOrderRepo.fetchBuyByType(mTypeId);

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

        if (mGrouping != Grouping::None)
            refreshGroupedData();

        endResetModel();
    }

    void ExternalOrderBuyModel::changeDeviationSource(DeviationSourceType type, double value)
    {
        beginResetModel();

        mDeviationType = type;
        mDeviationValue = value;

        endResetModel();
    }

    void ExternalOrderBuyModel::setGrouping(Grouping grouping)
    {
        beginResetModel();

        mGrouping = grouping;
        refreshGroupedData();

        endResetModel();
    }

    double ExternalOrderBuyModel::computeDeviation(const ExternalOrder &order) const
    {
        switch (mDeviationType) {
        case DeviationSourceType::Median:
            return (mMedianPrice == 0.) ? (0.) : ((order.getPrice() - mMedianPrice) / mMedianPrice);
        case DeviationSourceType::Best:
            return (mMaxPrice == 0.) ? (0.) : ((order.getPrice() - mMaxPrice) / mMaxPrice);
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

    void ExternalOrderBuyModel::refreshGroupedData()
    {
        switch (mGrouping) {
        case Grouping::Station:
            fillGroupedData<&ExternalOrder::getStationId>();
            break;
        case Grouping::System:
            fillGroupedData<&ExternalOrder::getSolarSystemId>();
            break;
        case Grouping::Region:
            fillGroupedData<&ExternalOrder::getRegionId>();
            break;
        default:
            break;
        }
    }

    QVariant ExternalOrderBuyModel::getUngroupedData(int column, int role, const ExternalOrder &order) const
    {
        switch (role) {
        case Qt::DisplayRole:
            {
                QLocale locale;

                switch (column) {
                case stationColumn:
                    return mDataProvider.getLocationName(order.getStationId());
                case deviationColumn:
                    return QString{"%1%2"}.arg(static_cast<int>(computeDeviation(order) * 100.)).arg(locale.percent());
                case priceColumn:
                    return locale.toCurrencyString(order.getPrice(), "ISK");
                case volumeColumn:
                    return QString{"%1/%2"}.arg(locale.toString(order.getVolumeRemaining())).arg(locale.toString(order.getVolumeEntered()));
                case rangeColumn:
                    {
                        const auto range = order.getRange();
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
                case minQunatityColumn:
                    return locale.toString(order.getMinVolume());
                case totalCostColumn:
                    return locale.toCurrencyString(order.getVolumeRemaining() * order.getPrice(), "ISK");
                case totalSizeColumn:
                    return QString{"%1m³"}.arg(locale.toString(order.getVolumeRemaining() * mDataProvider.getTypeVolume(order.getTypeId()), 'f', 2));
                case issuedColumn:
                    return TextUtils::dateTimeToString(order.getIssued().toLocalTime(), locale);
                case durationColumn:
                    {
                        const auto timeEnd = order.getIssued().addDays(order.getDuration()).toMSecsSinceEpoch() / 1000;
                        const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                        if (timeEnd > timeCur)
                            return TextUtils::secondsToString(timeEnd - timeCur);
                    }
                    break;
                case updatedColumn:
                    return TextUtils::dateTimeToString(order.getUpdateTime().toLocalTime(), locale);
                }
            }
            break;
        case Qt::UserRole:
            switch (column) {
            case stationColumn:
                return mDataProvider.getLocationName(order.getStationId());
            case deviationColumn:
                return computeDeviation(order);
            case priceColumn:
                return order.getPrice();
            case volumeColumn:
                return QVariantList{} << order.getVolumeRemaining() << order.getVolumeEntered();
            case rangeColumn:
                return order.getRange();
            case minQunatityColumn:
                return order.getMinVolume();
            case totalCostColumn:
                return order.getVolumeRemaining() * order.getPrice();
            case totalSizeColumn:
                return order.getVolumeRemaining() * mDataProvider.getTypeVolume(order.getTypeId());
            case issuedColumn:
                return order.getIssued();
            case durationColumn:
                {
                    const auto timeEnd = order.getIssued().addDays(order.getDuration()).toMSecsSinceEpoch() / 1000;
                    const auto timeCur = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;

                    if (timeEnd > timeCur)
                        return timeEnd - timeCur;
                }
                break;
            case updatedColumn:
                return order.getUpdateTime();
            }
            break;
        case Qt::ForegroundRole:
            if (column == totalCostColumn)
                return QColor{Qt::darkGreen};
            break;
        case Qt::BackgroundRole:
            if (mOwnOrders.find(order.getId()) != std::end(mOwnOrders))
                return QColor{255, 255, 128};
            break;
        case Qt::ToolTipRole:
            if (mOwnOrders.find(order.getId()) != std::end(mOwnOrders))
                return tr("Your order");
            break;
        case Qt::TextAlignmentRole:
            if (column == volumeColumn || column == deviationColumn)
                return Qt::AlignRight;
        }

        return QVariant{};
    }

    QVariant ExternalOrderBuyModel::getStationGroupedData(int column, int role, const GroupedData &data) const
    {
        if (column == groupByColumn)
        {
            switch (role) {
            case Qt::DisplayRole:
            case Qt::UserRole:
                return mDataProvider.getLocationName(data.mId);
            }

            return QVariant{};
        }

        return getGenericGroupedData(column, role, data);
    }

    QVariant ExternalOrderBuyModel::getSystemGroupedData(int column, int role, const GroupedData &data) const
    {
        if (column == groupByColumn)
        {
            switch (role) {
            case Qt::DisplayRole:
            case Qt::UserRole:
                return mDataProvider.getSolarSystemName(data.mId);
            }

            return QVariant{};
        }

        return getGenericGroupedData(column, role, data);
    }

    QVariant ExternalOrderBuyModel::getRegionGroupedData(int column, int role, const GroupedData &data) const
    {
        if (column == groupByColumn)
        {
            switch (role) {
            case Qt::DisplayRole:
            case Qt::UserRole:
                return mDataProvider.getRegionName(data.mId);
            }

            return QVariant{};
        }

        return getGenericGroupedData(column, role, data);
    }

    QVariant ExternalOrderBuyModel::getGenericGroupedData(int column, int role, const GroupedData &data) const
    {
        switch (role) {
        case Qt::DisplayRole:
            {
                QLocale locale;

                switch (column) {
                case lowestPriceColumn:
                    return locale.toCurrencyString(data.mLowestPrice, "ISK");
                case medianPriceColumn:
                    return locale.toCurrencyString(data.mMedianPrice, "ISK");
                case highestPriceColumn:
                    return locale.toCurrencyString(data.mHighestPrice, "ISK");
                case volumeColumn:
                    return QString{"%1/%2"}.arg(locale.toString(data.mVolumeRemaining)).arg(locale.toString(data.mVolumeEntered));
                case groupedTotalCostColumn:
                    return locale.toCurrencyString(data.mTotalCost, "ISK");
                case ordersColumn:
                    return locale.toString(data.mCount);
                case groupedTotalSizeColumn:
                    return QString{"%1m³"}.arg(locale.toString(data.mVolumeRemaining * mDataProvider.getTypeVolume(mTypeId), 'f', 2));
                }
            }
            break;
        case Qt::UserRole:
            switch (column) {
            case lowestPriceColumn:
                return data.mLowestPrice;
            case medianPriceColumn:
                return data.mMedianPrice;
            case highestPriceColumn:
                return data.mHighestPrice;
            case volumeColumn:
                return QVariantList{} << data.mVolumeRemaining << data.mVolumeEntered;
            case groupedTotalCostColumn:
                return data.mTotalCost;
            case ordersColumn:
                return data.mCount;
            case groupedTotalSizeColumn:
                return data.mVolumeRemaining * mDataProvider.getTypeVolume(mTypeId);
            }
            break;
        case Qt::ForegroundRole:
            if (column == groupedTotalCostColumn)
                return QColor{Qt::darkGreen};
            break;
        case Qt::TextAlignmentRole:
            if (column == volumeColumn)
                return Qt::AlignRight;
        }

        return QVariant{};
    }

    template<uint (ExternalOrder::* Func)() const>
    void ExternalOrderBuyModel::fillGroupedData()
    {
        mGroupedData.clear();

        std::unordered_map<uint, GroupedData> data;
        std::unordered_map<uint, std::vector<double>> prices;

        for (const auto &order : mOrders)
        {
            const auto id = (order.get()->*Func)();
            auto &curData = data[id];
            curData.mId = id;

            const auto price = order->getPrice();
            if (curData.mLowestPrice > price)
                curData.mLowestPrice = price;
            if (curData.mHighestPrice < price)
                curData.mHighestPrice = price;

            prices[id].emplace_back(price);

            curData.mVolumeEntered += order->getVolumeEntered();
            curData.mVolumeRemaining += order->getVolumeRemaining();
            curData.mTotalCost += order->getVolumeRemaining() * price;

            ++curData.mCount;

            curData.mTotalSize += order->getVolumeRemaining() * mDataProvider.getTypeVolume(order->getTypeId());
        }

        mGroupedData.reserve(data.size());

        for (auto &curData : data)
        {
            mGroupedData.emplace_back(std::move(curData.second));

            auto &curPrices = prices[curData.first];
            std::nth_element(std::begin(curPrices), std::next(std::begin(curPrices), curPrices.size() / 2), std::end(curPrices));

            mGroupedData.back().mMedianPrice = curPrices[curPrices.size() / 2];
        }
    }
}
