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
#include <set>

#include <QLocale>
#include <QColor>

#include <boost/range/adaptor/reversed.hpp>

#include "EveDataProvider.h"
#include "ExternalOrder.h"
#include "PriceUtils.h"
#include "TextUtils.h"

#include "InterRegionMarketDataModel.h"

namespace Evernus
{
    InterRegionMarketDataModel::InterRegionMarketDataModel(const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractTableModel{parent}
        , mDataProvider{dataProvider}
    {
    }

    int InterRegionMarketDataModel::columnCount(const QModelIndex &parent) const
    {
        return numColumns;
    }

    QVariant InterRegionMarketDataModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant{};

        const auto column = index.column();
        const auto &data = mData[index.row()];

        switch (role) {
        case Qt::DisplayRole:
            {
                QLocale locale;

                switch (column) {
                case nameColumn:
                    return mDataProvider.getTypeName(data.mId);
                case scoreColumn:
                    return locale.toString((data.mDstSellPrice - data.mSrcBuyPrice) * data.mVolume, 'f', 0);
                case srcRegionColumn:
                    return mDataProvider.getRegionName(data.mSrcRegion);
                case srcPriceColumn:
                    return QString{"%1 / %2"}.arg(TextUtils::currencyToString(data.mSrcBuyPrice, locale)).arg(TextUtils::currencyToString(data.mSrcSellPrice, locale));
                case dstRegionColumn:
                    return mDataProvider.getRegionName(data.mDstRegion);
                case dstPriceColumn:
                    return QString{"%1 / %2"}.arg(TextUtils::currencyToString(data.mDstBuyPrice, locale)).arg(TextUtils::currencyToString(data.mDstSellPrice, locale));
                case differenceColumn:
                    return TextUtils::currencyToString(data.mDstSellPrice - data.mSrcBuyPrice, locale);
                case volumeColumn:
                    return locale.toString(data.mVolume);
                case marginColumn:
                    return QString{"%1%2"}.arg(locale.toString(getMargin(data), 'f', 2)).arg(locale.percent());
                }
            }
            break;
        case Qt::UserRole:
            switch (column) {
            case nameColumn:
                return mDataProvider.getTypeName(data.mId);
            case scoreColumn:
                return (data.mDstSellPrice - data.mSrcBuyPrice) * data.mVolume;
            case srcRegionColumn:
                return mDataProvider.getRegionName(data.mSrcRegion);
            case srcPriceColumn:
                return (data.mSrcBuyPrice + data.mSrcSellPrice) / 2.;
            case dstRegionColumn:
                return mDataProvider.getRegionName(data.mDstRegion);
            case dstPriceColumn:
                return (data.mDstBuyPrice + data.mDstSellPrice) / 2.;
            case differenceColumn:
                return data.mDstSellPrice - data.mSrcBuyPrice;
            case volumeColumn:
                return data.mVolume;
            case marginColumn:
                return getMargin(data);
            }
            break;
        case Qt::UserRole + 1:
            switch (column) {
            case srcRegionColumn:
                return data.mSrcRegion;
            case dstRegionColumn:
                return data.mDstRegion;
            }
            break;
        case Qt::ForegroundRole:
            if (column == marginColumn)
                return TextUtils::getMarginColor(getMargin(data));
        }

        return QVariant{};
    }

    QVariant InterRegionMarketDataModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case nameColumn:
                return tr("Name");
            case scoreColumn:
                return tr("Score");
            case srcRegionColumn:
                return tr("Source");
            case srcPriceColumn:
                return tr("5% volume source price (b/s)");
            case dstRegionColumn:
                return tr("Destination");
            case dstPriceColumn:
                return tr("5% volume destination price (b/s)");
            case differenceColumn:
                return tr("Best difference");
            case volumeColumn:
                return tr("30-day avg. min. volume");
            case marginColumn:
                return tr("Margin");
            }
        }

        return QVariant{};
    }

    int InterRegionMarketDataModel::rowCount(const QModelIndex &parent) const
    {
        return static_cast<int>(mData.size());
    }

    void InterRegionMarketDataModel::setOrderData(const std::vector<ExternalOrder> &orders, const HistoryRegionMap &history)
    {
        beginResetModel();

        mData.clear();

        struct LowToHigh
        {
            bool operator ()(const ExternalOrder &a, const ExternalOrder &b) const noexcept
            {
                return a.getPrice() < b.getPrice();
            }
        };

        struct HighToLow
        {
            bool operator ()(const ExternalOrder &a, const ExternalOrder &b) const noexcept
            {
                return a.getPrice() > b.getPrice();
            }
        };

        RegionMap<TypeMap<std::multiset<std::reference_wrapper<const ExternalOrder>, LowToHigh>>> sellOrders;
        RegionMap<TypeMap<std::multiset<std::reference_wrapper<const ExternalOrder>, HighToLow>>> buyOrders;

        RegionMap<TypeMap<uint>> sellVolumes, buyVolumes;

        for (const auto &order : orders)
        {
            const auto typeId = order.getTypeId();
            const auto regionId = order.getRegionId();
            if (order.getType() == ExternalOrder::Type::Buy)
            {
                buyOrders[regionId][typeId].insert(std::cref(order));
                buyVolumes[regionId][typeId] += order.getVolumeRemaining();
            }
            else
            {
                sellOrders[regionId][typeId].insert(std::cref(order));
                sellVolumes[regionId][typeId] += order.getVolumeRemaining();
            }
        }

        auto calcPercentile = [](const auto &orders, uint maxVolume) {
            if (maxVolume == 0)
                maxVolume = 1;

            auto it = std::begin(orders);
            auto volume = 0u;
            auto result = 0.;

            while (volume < maxVolume && it != std::end(orders))
            {
                const auto orderVolume = it->get().getVolumeRemaining();
                const auto add = std::min(orderVolume, maxVolume - volume);

                volume += add;
                result += it->get().getPrice() * add;

                ++it;
            }

            return result / maxVolume;
        };

        const auto historyLimit = QDate::currentDate().addDays(-30);

        struct AggrTypeData
        {
            double mBuyPrice;
            double mSellPrice;
            uint mVolume;
        };

        RegionMap<TypeMap<AggrTypeData>> aggrTypeData;

        for (const auto regionHistory : history)
        {
            for (const auto type : regionHistory.second)
            {
                AggrTypeData data;
                data.mBuyPrice = calcPercentile(buyOrders[regionHistory.first][type.first], buyVolumes[regionHistory.first][type.first] * 0.05);
                data.mSellPrice = calcPercentile(sellOrders[regionHistory.first][type.first], sellVolumes[regionHistory.first][type.first] * 0.05);

                for (const auto &timePoint : boost::adaptors::reverse(type.second))
                {
                    if (timePoint.first < historyLimit)
                        break;

                    data.mVolume += timePoint.second.mVolume;
                }

                data.mVolume /= 30;

                aggrTypeData[regionHistory.first].emplace(type.first, std::move(data));
            }
        }

        for (const auto srcRegion : aggrTypeData)
        {
            for (const auto type : srcRegion.second)
            {
                for (const auto dstRegion : aggrTypeData)
                {
                    if (dstRegion.first == srcRegion.first)
                        continue;

                    const auto dstData = dstRegion.second.find(type.first);
                    if (dstData == std::end(dstRegion.second))
                        continue;

                    TypeData data;
                    data.mId = type.first;
                    data.mSrcBuyPrice = type.second.mBuyPrice;
                    data.mSrcSellPrice = type.second.mSellPrice;
                    data.mDstBuyPrice = dstData->second.mBuyPrice;
                    data.mDstSellPrice = dstData->second.mSellPrice;
                    data.mVolume = std::min(type.second.mVolume, dstData->second.mVolume);
                    data.mSrcRegion = srcRegion.first;
                    data.mDstRegion = dstRegion.first;

                    mData.emplace_back(std::move(data));
                }
            }
        }

        endResetModel();
    }

    void InterRegionMarketDataModel::setCharacter(const std::shared_ptr<Character> &character)
    {
        beginResetModel();
        mCharacter = character;
        endResetModel();
    }

    EveType::IdType InterRegionMarketDataModel::getTypeId(const QModelIndex &index) const
    {
        if (!index.isValid())
            return EveType::invalidId;

        return mData[index.row()].mId;
    }

    int InterRegionMarketDataModel::getSrcRegionColumn()
    {
        return srcRegionColumn;
    }

    int InterRegionMarketDataModel::getDstRegionColumn()
    {
        return dstRegionColumn;
    }

    double InterRegionMarketDataModel::getMargin(const TypeData &data) const
    {
        if (!mCharacter || qFuzzyIsNull(data.mDstSellPrice))
            return 0.;

        const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
        return PriceUtils::getMargin(data.mSrcBuyPrice, data.mDstSellPrice, taxes);
    }
}
