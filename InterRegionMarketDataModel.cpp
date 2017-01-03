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

#include <QEventLoop>
#include <QSettings>
#include <QLocale>
#include <QColor>

#include <boost/range/adaptor/reversed.hpp>

#include "MarketAnalysisSettings.h"
#include "EveDataProvider.h"
#include "ExternalOrder.h"
#include "PriceUtils.h"
#include "MathUtils.h"
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
                    return locale.toString(data.mDifference * data.mVolume, 'f', 0);
                case srcRegionColumn:
                    return mDataProvider.getRegionName(data.mSrcRegion);
                case srcPriceColumn:
                    return QString{"%1 / %2"}.arg(TextUtils::currencyToString(data.mSrcBuyPrice, locale)).arg(TextUtils::currencyToString(data.mSrcSellPrice, locale));
                case dstRegionColumn:
                    return mDataProvider.getRegionName(data.mDstRegion);
                case dstPriceColumn:
                    return QString{"%1 / %2"}.arg(TextUtils::currencyToString(data.mDstBuyPrice, locale)).arg(TextUtils::currencyToString(data.mDstSellPrice, locale));
                case differenceColumn:
                    return TextUtils::currencyToString(data.mDifference, locale);
                case volumeColumn:
                    return locale.toString(data.mVolume);
                case marginColumn:
                    return QString{"%1%2"}.arg(locale.toString(data.mMargin, 'f', 2)).arg(locale.percent());
                }
            }
            break;
        case Qt::UserRole:
            switch (column) {
            case nameColumn:
                return mDataProvider.getTypeName(data.mId);
            case scoreColumn:
                return data.mDifference * data.mVolume;
            case srcRegionColumn:
                return mDataProvider.getRegionName(data.mSrcRegion);
            case srcPriceColumn:
                return (data.mSrcBuyPrice + data.mSrcSellPrice) / 2.;
            case dstRegionColumn:
                return mDataProvider.getRegionName(data.mDstRegion);
            case dstPriceColumn:
                return (data.mDstBuyPrice + data.mDstSellPrice) / 2.;
            case differenceColumn:
                return data.mDifference;
            case volumeColumn:
                return data.mVolume;
            case marginColumn:
                return data.mMargin;
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
                return TextUtils::getMarginColor(data.mMargin);
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

    void InterRegionMarketDataModel::setOrderData(const std::vector<ExternalOrder> &orders,
                                                  const HistoryRegionMap &history,
                                                  quint64 srcStation,
                                                  quint64 dstStation)
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

        QEventLoop loop;

        const auto srcRegionId = (srcStation == 0) ? (0u) : (mDataProvider.getStationRegionId(srcStation));
        const auto dstRegionId = (dstStation == 0) ? (0u) : (mDataProvider.getStationRegionId(dstStation));

        for (const auto &order : orders)
        {
            const auto typeId = order.getTypeId();
            const auto regionId = order.getRegionId();
            const auto stationId = order.getStationId();

            if ((srcRegionId != 0 && srcRegionId == regionId && stationId != srcStation) ||
                (dstRegionId != 0 && dstRegionId == regionId && stationId != dstStation))
            {
                continue;
            }

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

            loop.processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        const auto historyLimit = QDate::currentDate().addDays(-30);

        struct AggrTypeData
        {
            double mBuyPrice = 0.;
            double mSellPrice = 0.;
            uint mVolume = 0;
        };

        RegionMap<TypeMap<AggrTypeData>> aggrTypeData;

        for (const auto regionHistory : history)
        {
            for (const auto type : regionHistory.second)
            {
                AggrTypeData data;
                auto avgPrice30 = 0.;

                for (const auto &timePoint : boost::adaptors::reverse(type.second))
                {
                    if (timePoint.first < historyLimit)
                        break;

                    data.mVolume += timePoint.second.mVolume;
                    avgPrice30 += timePoint.second.mAvgPrice;
                }

                avgPrice30 /= 30.;

                data.mVolume /= 30;
                data.mBuyPrice = MathUtils::calcPercentile(buyOrders[regionHistory.first][type.first],
                                                           buyVolumes[regionHistory.first][type.first] * 0.05,
                                                           avgPrice30,
                                                           mDiscardBogusOrders,
                                                           mBogusOrderThreshold);
                data.mSellPrice = MathUtils::calcPercentile(sellOrders[regionHistory.first][type.first],
                                                            sellVolumes[regionHistory.first][type.first] * 0.05,
                                                            avgPrice30,
                                                            mDiscardBogusOrders,
                                                            mBogusOrderThreshold);

                aggrTypeData[regionHistory.first].emplace(type.first, std::move(data));

                loop.processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        }

        PriceUtils::Taxes taxes;

        QSettings settings;
        const auto useSkillsForDifference = mCharacter && settings.value(
            MarketAnalysisSettings::useSkillsForDifferenceKey, MarketAnalysisSettings::useSkillsForDifferenceDefault).toBool();

        if (useSkillsForDifference)
            taxes = PriceUtils::calculateTaxes(*mCharacter);

        for (const auto srcRegion : aggrTypeData)
        {
            if (srcRegionId != 0 && srcRegion.first != srcRegionId)
                continue;

            for (const auto type : srcRegion.second)
            {
                for (const auto dstRegion : aggrTypeData)
                {
                    if ((dstRegionId != 0 && dstRegion.first != dstRegionId) || (dstRegion.first == srcRegion.first))
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

                    if (useSkillsForDifference)
                    {
                        const auto realSellPrice = PriceUtils::getRevenue(data.mDstSellPrice, taxes);
                        const auto realBuyPrice = PriceUtils::getCoS(data.mSrcBuyPrice, taxes);

                        data.mDifference = realSellPrice - realBuyPrice;
                        data.mMargin = (qFuzzyIsNull(realSellPrice)) ? (0.) : (100. * data.mDifference / realSellPrice);
                    }
                    else
                    {
                        data.mDifference = data.mDstSellPrice - data.mSrcBuyPrice;
                        data.mMargin = (qFuzzyIsNull(data.mDstSellPrice)) ? (0.) : (100. * data.mDifference / data.mDstSellPrice);
                    }

                    mData.emplace_back(std::move(data));

                    loop.processEvents(QEventLoop::ExcludeUserInputEvents);
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

    void InterRegionMarketDataModel::discardBogusOrders(bool flag)
    {
        mDiscardBogusOrders = flag;
    }

    void InterRegionMarketDataModel::setBogusOrderThreshold(double value)
    {
        mBogusOrderThreshold = value;
    }

    EveType::IdType InterRegionMarketDataModel::getTypeId(const QModelIndex &index) const
    {
        if (!index.isValid())
            return EveType::invalidId;

        return mData[index.row()].mId;
    }

    Character::IdType InterRegionMarketDataModel::getOwnerId(const QModelIndex &index) const
    {
        return (mCharacter) ? (mCharacter->getId()) : (Character::invalidId);
    }

    void InterRegionMarketDataModel::reset()
    {
        beginResetModel();
        mData.clear();
        endResetModel();
    }

    int InterRegionMarketDataModel::getSrcRegionColumn()
    {
        return srcRegionColumn;
    }

    int InterRegionMarketDataModel::getDstRegionColumn()
    {
        return dstRegionColumn;
    }

    int InterRegionMarketDataModel::getVolumeColumn()
    {
        return volumeColumn;
    }

    int InterRegionMarketDataModel::getMarginColumn()
    {
        return marginColumn;
    }
}
