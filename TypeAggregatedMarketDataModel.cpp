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
#include <unordered_set>
#include <functional>
#include <set>

#include <QSettings>
#include <QLocale>
#include <QColor>
#include <QIcon>

#include <boost/range/adaptor/reversed.hpp>

#include "MarketAnalysisSettings.h"
#include "EveDataProvider.h"
#include "ExternalOrder.h"
#include "PriceUtils.h"
#include "MathUtils.h"
#include "TextUtils.h"

#include "TypeAggregatedMarketDataModel.h"

namespace Evernus
{
    TypeAggregatedMarketDataModel::TypeAggregatedMarketDataModel(const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractTableModel{parent}
        , mDataProvider{dataProvider}
    {
    }

    int TypeAggregatedMarketDataModel::columnCount(const QModelIndex &parent) const
    {
        return numColumns;
    }

    QVariant TypeAggregatedMarketDataModel::data(const QModelIndex &index, int role) const
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
                case srcPriceColumn:
                    return TextUtils::currencyToString((mSrcPriceType == PriceType::Sell) ? (data.mSellPrice) : (data.mBuyPrice), locale);
                case dstPriceColumn:
                    return TextUtils::currencyToString((mDstPriceType == PriceType::Sell) ? (data.mSellPrice) : (data.mBuyPrice), locale);
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
            case srcPriceColumn:
                return (mSrcPriceType == PriceType::Sell) ? (data.mSellPrice) : (data.mBuyPrice);
            case dstPriceColumn:
                return (mDstPriceType == PriceType::Sell) ? (data.mSellPrice) : (data.mBuyPrice);
            case differenceColumn:
                return data.mDifference;
            case volumeColumn:
                return data.mVolume;
            case marginColumn:
                return data.mMargin;
            }
            break;
        case Qt::ToolTipRole:
            if (column == nameColumn)
                return tr("Double-click for detailed market information.");
            break;
        case Qt::DecorationRole:
            if (column == nameColumn)
                return QIcon{":/images/chart_curve.png"};
            break;
        case Qt::ForegroundRole:
            if (column == marginColumn)
                return TextUtils::getMarginColor(data.mMargin);
        }

        return QVariant{};
    }

    QVariant TypeAggregatedMarketDataModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case nameColumn:
                return tr("Name");
            case scoreColumn:
                return tr("Score");
            case srcPriceColumn:
                return tr("5% volume source price");
            case dstPriceColumn:
                return tr("5% volume destination price");
            case differenceColumn:
                return tr("Difference");
            case volumeColumn:
                return tr("30-day avg. volume");
            case marginColumn:
                return tr("Margin");
            }
        }

        return QVariant{};
    }

    int TypeAggregatedMarketDataModel::rowCount(const QModelIndex &parent) const
    {
        return static_cast<int>(mData.size());
    }

    void TypeAggregatedMarketDataModel
    ::setOrderData(const std::vector<ExternalOrder> &orders, const HistoryMap &history, uint region, PriceType srcType, PriceType dstType, uint solarSystem)
    {
        beginResetModel();

        mData.clear();

        mSrcPriceType = srcType;
        mDstPriceType = dstType;

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

        TypeMap<std::multiset<std::reference_wrapper<const ExternalOrder>, LowToHigh>> sellOrders;
        TypeMap<std::multiset<std::reference_wrapper<const ExternalOrder>, HighToLow>> buyOrders;

        TypeMap<uint> sellVolumes, buyVolumes;

        std::unordered_set<EveType::IdType> usedTypes;

        for (const auto &order : orders)
        {
            if (order.getRegionId() != region || (solarSystem != 0 && order.getSolarSystemId() != solarSystem))
                continue;

            const auto typeId = order.getTypeId();
            if (order.getType() == ExternalOrder::Type::Buy)
            {
                buyOrders[typeId].insert(std::cref(order));
                buyVolumes[typeId] += order.getVolumeRemaining();
            }
            else
            {
                sellOrders[typeId].insert(std::cref(order));
                sellVolumes[typeId] += order.getVolumeRemaining();
            }

            usedTypes.insert(typeId);
        }

        const auto historyLimit = QDate::currentDate().addDays(-30);
        PriceUtils::Taxes taxes;

        QSettings settings;
        const auto useSkillsForDifference = mCharacter && settings.value(
            MarketAnalysisSettings::useSkillsForDifferenceKey, MarketAnalysisSettings::useSkillsForDifferenceDefault).toBool();

        if (useSkillsForDifference)
            taxes = PriceUtils::calculateTaxes(*mCharacter);

        for (const auto type : usedTypes)
        {
            TypeData data;
            auto avgPrice30 = 0.;

            const auto typeHistory = history.find(type);
            if (typeHistory != std::end(history))
            {
                for (const auto &timePoint : boost::adaptors::reverse(typeHistory->second))
                {
                    if (timePoint.first < historyLimit)
                        break;

                    data.mVolume += timePoint.second.mVolume;
                    avgPrice30 += timePoint.second.mAvgPrice;
                }

                data.mVolume /= 30;
                avgPrice30 /= 30.;
            }

            data.mId = type;
            data.mBuyPrice = MathUtils::calcPercentile(buyOrders[type],
                                                       buyVolumes[type] * 0.05,
                                                       avgPrice30,
                                                       mDiscardBogusOrders,
                                                       mBogusOrderThreshold);
            data.mSellPrice = MathUtils::calcPercentile(sellOrders[type],
                                                        sellVolumes[type] * 0.05,
                                                        avgPrice30,
                                                        mDiscardBogusOrders,
                                                        mBogusOrderThreshold);

            double realSellPrice, realBuyPrice;
            if (useSkillsForDifference)
            {
                realSellPrice = (mDstPriceType == PriceType::Sell) ? (PriceUtils::getRevenue(data.mSellPrice, taxes)) : (PriceUtils::getCoS(data.mBuyPrice, taxes));
                realBuyPrice = (mSrcPriceType == PriceType::Buy) ? (PriceUtils::getCoS(data.mBuyPrice, taxes)) : (PriceUtils::getRevenue(data.mSellPrice, taxes));
            }
            else
            {
                realSellPrice = (mDstPriceType == PriceType::Sell) ? (data.mSellPrice) : (data.mBuyPrice);
                realBuyPrice = (mSrcPriceType == PriceType::Buy) ? (data.mBuyPrice) : (data.mSellPrice);
            }

            data.mDifference = realSellPrice - realBuyPrice;
            data.mMargin = (qFuzzyIsNull(realSellPrice)) ? (0.) : (100. * data.mDifference / realSellPrice);

            mData.emplace_back(std::move(data));
        }

        endResetModel();
    }

    void TypeAggregatedMarketDataModel::setCharacter(const std::shared_ptr<Character> &character)
    {
        beginResetModel();
        mCharacter = character;
        endResetModel();
    }

    void TypeAggregatedMarketDataModel::discardBogusOrders(bool flag) noexcept
    {
        mDiscardBogusOrders = flag;
    }

    void TypeAggregatedMarketDataModel::setBogusOrderThreshold(double value) noexcept
    {
        mBogusOrderThreshold = value;
    }

    EveType::IdType TypeAggregatedMarketDataModel::getTypeId(const QModelIndex &index) const
    {
        if (!index.isValid())
            return EveType::invalidId;

        return mData[index.row()].mId;
    }

    Character::IdType TypeAggregatedMarketDataModel::getOwnerId(const QModelIndex &index) const
    {
        return (mCharacter) ? (mCharacter->getId()) : (Character::invalidId);
    }

    int TypeAggregatedMarketDataModel::getScoreColumn() noexcept
    {
        return scoreColumn;
    }

    int TypeAggregatedMarketDataModel::getVolumeColumn() noexcept
    {
        return volumeColumn;
    }

    int TypeAggregatedMarketDataModel::getMarginColumn() noexcept
    {
        return marginColumn;
    }

    int TypeAggregatedMarketDataModel::getBuyPriceColumn() noexcept
    {
        return srcPriceColumn;
    }

    int TypeAggregatedMarketDataModel::getSellPriceColumn() noexcept
    {
        return dstPriceColumn;
    }
}
