/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for mScrapmetal details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <unordered_map>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <set>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/scope_exit.hpp>

#include <QtConcurrent>

#include <QCoreApplication>
#include <QColor>
#include <QDebug>

#include "EveDataProvider.h"
#include "ExternalOrder.h"
#include "PriceUtils.h"
#include "TextUtils.h"

#include "ScrapmetalReprocessingArbitrageModel.h"

namespace Evernus
{
    ScrapmetalReprocessingArbitrageModel::ScrapmetalReprocessingArbitrageModel(const EveDataProvider &dataProvider, QObject *parent)
        : ReprocessingArbitrageModel{parent}
        , ModelWithTypes{}
        , mDataProvider{dataProvider}
    {
        insertOreGroup(QStringLiteral("Arkonor"));
        insertOreGroup(QStringLiteral("Bistot"));
        insertOreGroup(QStringLiteral("Crokite"));
        insertOreGroup(QStringLiteral("Dark Ochre"));
        insertOreGroup(QStringLiteral("Gneiss"));
        insertOreGroup(QStringLiteral("Hedbergite"));
        insertOreGroup(QStringLiteral("Hemorphite"));
        insertOreGroup(QStringLiteral("Ice"));
        insertOreGroup(QStringLiteral("Jaspet"));
        insertOreGroup(QStringLiteral("Kernite"));
        insertOreGroup(QStringLiteral("Mercoxit"));
        insertOreGroup(QStringLiteral("Omber"));
        insertOreGroup(QStringLiteral("Plagioclase"));
        insertOreGroup(QStringLiteral("Pyroxeres"));
        insertOreGroup(QStringLiteral("Scordite"));
        insertOreGroup(QStringLiteral("Spodumain"));
        insertOreGroup(QStringLiteral("Veldspar"));
    }

    int ScrapmetalReprocessingArbitrageModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return numColumns;
    }

    QVariant ScrapmetalReprocessingArbitrageModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return {};

        const auto column = index.column();
        const auto &data = mData[index.row()];

        switch (role) {
        case Qt::DisplayRole:
            {
                QLocale locale;

                switch (column) {
                case nameColumn:
                    return mDataProvider.getTypeName(data.mId);
                case volumeColumn:
                    return locale.toString(data.mVolume);
                case totalProfitColumn:
                    return TextUtils::currencyToString(data.mTotalProfit, locale);
                case totalCostColumn:
                    return TextUtils::currencyToString(data.mTotalCost, locale);
                case differenceColumn:
                    return TextUtils::currencyToString(data.mTotalProfit - data.mTotalCost, locale);
                case marginColumn:
                    return QStringLiteral("%1%2").arg(locale.toString(data.mMargin, 'f', 2)).arg(locale.percent());
                }
            }
            break;
        case Qt::UserRole:
            switch (column) {
            case nameColumn:
                return mDataProvider.getTypeName(data.mId);
            case volumeColumn:
                return data.mVolume;
            case totalProfitColumn:
                return data.mTotalProfit;
            case totalCostColumn:
                return data.mTotalCost;
            case differenceColumn:
                return data.mTotalProfit - data.mTotalCost;
            case marginColumn:
                return data.mMargin;
            }
            break;
        case Qt::ForegroundRole:
            if (column == marginColumn)
                return TextUtils::getMarginColor(data.mMargin);
        }

        return {};
    }

    QVariant ScrapmetalReprocessingArbitrageModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case nameColumn:
                return tr("Name");
            case volumeColumn:
                return tr("Volume");
            case totalProfitColumn:
                return tr("Total profit");
            case totalCostColumn:
                return tr("Total cost");
            case differenceColumn:
                return tr("Difference");
            case marginColumn:
                return tr("Margin");
            }
        }

        return {};
    }

    int ScrapmetalReprocessingArbitrageModel::rowCount(const QModelIndex &parent) const
    {
        return (parent.isValid()) ? (0) : (static_cast<int>(mData.size()));
    }

    EveType::IdType ScrapmetalReprocessingArbitrageModel::getTypeId(const QModelIndex &index) const
    {
        if (!index.isValid())
            return EveType::invalidId;

        return mData[index.row()].mId;
    }

    void ScrapmetalReprocessingArbitrageModel::setCharacter(std::shared_ptr<Character> character)
    {
        beginResetModel();
        mCharacter = std::move(character);
        endResetModel();
    }

    void ScrapmetalReprocessingArbitrageModel::setOrderData(const std::vector<ExternalOrder> &orders,
                                                            PriceType dstPriceType,
                                                            const RegionList &srcRegions,
                                                            const RegionList &dstRegions,
                                                            quint64 srcStation,
                                                            quint64 dstStation,
                                                            bool useStationTax,
                                                            bool ignScrapmetalMinVolume,
                                                            bool onlyHighSec,
                                                            double baseYield,
                                                            double sellVolumeLimit)
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        mData.clear();

        if (!mCharacter)
            return;

        const auto reprocessingSkills = mCharacter->getReprocessingSkills();
        const auto reprocessingYield = baseYield * (1 + reprocessingSkills.mScrapmetalProcessing * 0.02);

        const auto dstSystem = (dstStation == 0) ? (0u) : (mDataProvider.getStationSolarSystemId(dstStation));

        const auto taxes = PriceUtils::calculateTaxes(*mCharacter);
        const auto stationTax = 1 - std::max(0., 5. - mCharacter->getCorpStanding() * 0.75) / 100.;

        const auto allSrcRegions = srcRegions.find(0) != std::end(srcRegions);
        const auto allDstRegions = dstRegions.find(0) != std::end(dstRegions);

        const auto isValidRegion = [&](auto allFlag, const auto &regions, const auto &order) {
            return allFlag || regions.find(order.getRegionId()) != std::end(regions);
        };

        const auto isValidStation = [&](auto stationId, const auto &order) {
            return stationId == 0 || order.getStationId() == stationId;
        };

        const auto isSrcOrder = [&](const auto &order) {
            return isValidRegion(allSrcRegions, srcRegions, order) &&
                   isValidStation(srcStation, order);
        };

        const auto canSellToOrder = [=](const auto &order) {
            return (dstSystem == 0) ||
                   (order.getRange() == ExternalOrder::rangeStation && dstStation == order.getStationId()) ||
                   (mDataProvider.getDistance(dstSystem, order.getSolarSystemId()) <= static_cast<uint>(order.getRange()));
        };

        const auto isDstOrder = [&](const auto &order) {
            return (order.getType() == dstPriceType) &&
                   (isValidRegion(allDstRegions, dstRegions, order)) &&
                   (
                       (dstPriceType == PriceType::Sell && isValidStation(dstStation, order)) ||
                       (dstPriceType == PriceType::Buy && canSellToOrder(order))
                   );
        };

        const auto orderFilter = [&](const auto &order) {
            return (isSrcOrder(order) || isDstOrder(order)) &&
                   (!ignScrapmetalMinVolume || order.getMinVolume() <= 1) &&
                   (!onlyHighSec || mDataProvider.getSolarSystemSecurityStatus(order.getSolarSystemId()) >= 0.5);
        };

        EveDataProvider::TypeList reprocessingTypes;

        std::unordered_map<EveType::IdType, std::multiset<ExternalOrder, ExternalOrder::LowToHigh>> sellMap;
        std::unordered_map<EveType::IdType, std::multiset<std::reference_wrapper<const ExternalOrder>, ExternalOrder::HighToLow>> buyMap;
        for (const auto &order : orders | boost::adaptors::filtered(orderFilter))
        {
            const auto typeId = order.getTypeId();
            if (isSrcOrder(order))
            {
                sellMap[typeId].emplace(order);
                reprocessingTypes.emplace(typeId);
            }
            if (isDstOrder(order))
                buyMap[typeId].emplace(std::cref(order));
        }

        const auto &aggregatedReprocessingInfo = mDataProvider.getTypeReprocessingInfo(reprocessingTypes);

        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        // for given type, try to find arbitrage opportunities from source orders to dst orders
        // we have 2 versions to avoid branching logic - selling to buy orders and using sell orders

        struct UsedOrder
        {
            uint mVolume;
            double mPrice;
        };

        // buy/sell a volume of stuff from orders
        const auto fillOrders = [](auto &orders, auto volume) {
            std::vector<UsedOrder> usedOrders;
            for (auto &order : orders)
            {
                const auto orderVolume = order.getVolumeRemaining();
                if (volume >= order.getMinVolume() && orderVolume > 0)
                {
                    const auto amount = std::min(orderVolume, volume);
                    volume -= amount;

                    // NOTE: we're casting away const, but not modifying the actual set key
                    // looks dirty, but there's no partial constness
                    const_cast<ExternalOrder &>(order).setVolumeRemaining(orderVolume - amount);

                    UsedOrder used{amount, order.getPrice()};
                    usedOrders.emplace_back(used);

                    if (volume == 0)
                        break;
                }
            }

            return usedOrders;
        };

        // NOTE: using std::function because QtConcurrent::mapped cannot infer the result type properly
        const std::function<ItemData (const decltype(sellMap)::value_type &)> findArbitrageForBuy = [&](const auto &sellOrderList) {
            Q_ASSERT(dstPriceType == PriceType::Buy);

            const auto reprocessingInfo = aggregatedReprocessingInfo.find(sellOrderList.first);
            if (reprocessingInfo == std::end(aggregatedReprocessingInfo) ||
                mOreGroups.find(reprocessingInfo->second.mGroupId) != std::end(mOreGroups))
            {
                return ItemData{};
            }

            qDebug() << "Finding arbitrage opportunities for" << sellOrderList.first;

            // copy buy map locally so we can modify volumes
            std::unordered_map<EveType::IdType, std::vector<ExternalOrder>> localBuyMap;
            for (const auto &material : reprocessingInfo->second.mMaterials)
            {
                const auto buyOrderList = buyMap.find(material.mMaterialId);
                if (buyOrderList == std::end(buyMap))
                    continue;

                localBuyMap.emplace(std::piecewise_construct,
                                    std::forward_as_tuple(material.mMaterialId),
                                    std::forward_as_tuple(std::begin(buyOrderList->second), std::end(buyOrderList->second)));
            }

            const auto requiredVolume = reprocessingInfo->second.mPortionSize;

            quint64 totalVolume = 0u;
            auto totalIncome = 0.;
            auto totalCost = 0.;

            // keep buying and selling until no scrapmetal orders are left or we stop making profit
            while (true)
            {
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

                // unsychronized access but that's ok, since only one thread touches given type orders
                const auto bought = fillOrders(sellOrderList.second, requiredVolume);
                if (bought.empty()) // no volume to buy
                    break;

                auto cost = std::accumulate(std::begin(bought), std::end(bought), 0., [&](auto total, const auto &order) {
                    return order.mVolume * PriceUtils::getCoS(order.mPrice, taxes, false) + total;
                });

                auto income = 0.;

                // try to sell all the refined goods
                for (const auto &material : reprocessingInfo->second.mMaterials)
                {
                    const uint sellVolume = reprocessingYield * material.mQuantity;

                    const auto buyOrderList = localBuyMap.find(material.mMaterialId);
                    if (buyOrderList == std::end(localBuyMap))   // can't sell this one, maybe there's still profit to be made
                        continue;

                    const auto sold = fillOrders(buyOrderList->second, sellVolume);

                    // cannot sell some stuff, so let's advance in hope we turn in a profit from other materials
                    if (sold.empty())
                        continue;

                    income += std::accumulate(std::begin(sold), std::end(sold), 0., [&](auto total, const auto &order) {
                        return order.mVolume * PriceUtils::getRevenue(order.mPrice, taxes, false) + total;
                    });

                    if (useStationTax)
                    {
                        cost += stationTax * std::accumulate(std::begin(sold), std::end(sold), 0., [&](auto total, const auto &order) {
                            return order.mVolume * order.mPrice + total;
                        });
                    }
                }

                if (income > cost)
                {
                    totalIncome += income;
                    totalCost += cost;
                    totalVolume += requiredVolume;
                }
                else
                {
                    // we stopped being profitable
                    break;
                }
            }

            qDebug() << "Done finding arbitrage opportunities for" << reprocessingInfo->first;

            // discard unprofitable
            if (totalCost >= totalIncome)
                return ItemData{};

            ItemData data;
            data.mId = sellOrderList.first;
            data.mTotalProfit = totalIncome;
            data.mTotalCost = totalCost;
            data.mVolume = totalVolume;

            if (!qFuzzyIsNull(data.mTotalCost))
                data.mMargin = 100. * (data.mTotalProfit - data.mTotalCost) / data.mTotalCost;

            return data;
        };

        const std::function<ItemData (const decltype(sellMap)::value_type &)> findArbitrageForSell = [&](const auto &sellOrderList) {
            Q_ASSERT(dstPriceType == PriceType::Sell);

            const auto reprocessingInfo = aggregatedReprocessingInfo.find(sellOrderList.first);
            if (reprocessingInfo == std::end(aggregatedReprocessingInfo) ||
                mOreGroups.find(reprocessingInfo->second.mGroupId) != std::end(mOreGroups))
            {
                return ItemData{};
            }

            qDebug() << "Finding arbitrage opportunities for" << sellOrderList.first;

            struct MaterialData
            {
                double mPrice;
                quint64 mVolume;
            };

            // find dst prices and volumes
            std::unordered_map<EveType::IdType, MaterialData> dstPrices;
            for (const auto &material : reprocessingInfo->second.mMaterials)
            {
                const auto dstOrderList = buyMap.find(material.mMaterialId);
                if (dstOrderList == std::end(buyMap) || dstOrderList->second.empty())   // can't sell this one, maybe there's still profit to be made
                    continue;

                // compute our dst limit order price
                auto &data = dstPrices[material.mMaterialId];
                data.mPrice = std::rbegin(dstOrderList->second)->get().getPrice() - PriceUtils::getPriceDelta();
                data.mVolume = std::accumulate(std::begin(dstOrderList->second),
                                               std::end(dstOrderList->second),
                                               0u,
                                               [](auto total, const auto &order) {
                    return total + order.get().getVolumeRemaining();
                }) * sellVolumeLimit;
            }

            const auto requiredVolume = reprocessingInfo->second.mPortionSize;

            quint64 totalVolume = 0u;
            auto totalIncome = 0.;
            auto totalCost = 0.;

            // keep buying and selling until no mScrapmetal orders are left, volume is exhausted or we stop making profit
            while (true)
            {
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

                // unsychronized access but that's ok, since only one thread touches given type orders
                const auto bought = fillOrders(sellOrderList.second, requiredVolume);
                if (bought.empty()) // no volume to buy
                    break;

                auto cost = std::accumulate(std::begin(bought), std::end(bought), 0., [&](auto total, const auto &order) {
                    return order.mVolume * PriceUtils::getCoS(order.mPrice, taxes, false) + total;
                });

                auto income = 0.;

                // try to sell all the refined goods
                for (const auto &material : reprocessingInfo->second.mMaterials)
                {
                    auto &dstData = dstPrices[material.mMaterialId];

                    const quint64 sellVolume = reprocessingYield * material.mQuantity;
                    const auto amount = std::min(sellVolume, dstData.mVolume);
                    if (amount == 0)
                        continue;

                    dstData.mVolume -= amount;
                    totalVolume += amount;

                    const auto price = dstData.mPrice;

                    income += PriceUtils::getRevenue(price, taxes) * amount;

                    if (useStationTax)
                        cost += stationTax * price * amount;
                }

                if (income > cost)
                {
                    totalIncome += income;
                    totalCost += cost;
                }
                else
                {
                    // we stopped being profitable
                    break;
                }
            }

            qDebug() << "Done finding arbitrage opportunities for" << reprocessingInfo->first;

            // discard unprofitable
            if (totalCost >= totalIncome)
                return ItemData{};

            ItemData data;
            data.mId = sellOrderList.first;
            data.mTotalProfit = totalIncome;
            data.mTotalCost = totalCost;
            data.mVolume = totalVolume;

            if (!qFuzzyIsNull(data.mTotalCost))
                data.mMargin = 100. * (data.mTotalProfit - data.mTotalCost) / data.mTotalCost;

            return data;
        };

        // fill our destination collection
        const auto fillData = [](auto &result, const auto &itemData) {
            if (itemData.mId != EveType::invalidId)
                result.emplace_back(itemData);
        };

        // concurrently check for all arbitrage opportunities
        mData = QtConcurrent::blockingMappedReduced<decltype(mData)>(sellMap,
                                                                     (dstPriceType == PriceType::Buy) ? (findArbitrageForBuy) : (findArbitrageForSell),
                                                                     fillData);
    }

    void ScrapmetalReprocessingArbitrageModel::reset()
    {
        beginResetModel();
        mData.clear();
        endResetModel();
    }

    void ScrapmetalReprocessingArbitrageModel::insertOreGroup(const QString &groupName)
    {
        const auto groupId = mDataProvider.getGroupId(groupName);
        if (groupId == 0)
            throw std::runtime_error{"Cannot find group id for: " + groupName.toStdString()};

        mOreGroups.emplace(groupId);
    }
}
