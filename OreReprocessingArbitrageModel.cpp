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
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <mutex>
#include <set>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/scope_exit.hpp>

#include <QtConcurrent>
#include <QDebug>

#include "EveDataProvider.h"
#include "ExternalOrder.h"

#include "OreReprocessingArbitrageModel.h"

namespace Evernus
{
    OreReprocessingArbitrageModel::OreReprocessingArbitrageModel(const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractTableModel{parent}
        , ModelWithTypes{}
        , mDataProvider{dataProvider}
    {
        insertSkillMapping(QStringLiteral("Arkonor"), &CharacterData::ReprocessingSkills::mArkonorProcessing);
        insertSkillMapping(QStringLiteral("Bistot"), &CharacterData::ReprocessingSkills::mBistotProcessing);
        insertSkillMapping(QStringLiteral("Crokite"), &CharacterData::ReprocessingSkills::mCrokiteProcessing);
        insertSkillMapping(QStringLiteral("Dark Ochre"), &CharacterData::ReprocessingSkills::mDarkOchreProcessing);
        insertSkillMapping(QStringLiteral("Gneiss"), &CharacterData::ReprocessingSkills::mGneissProcessing);
        insertSkillMapping(QStringLiteral("Hedbergite"), &CharacterData::ReprocessingSkills::mHedbergiteProcessing);
        insertSkillMapping(QStringLiteral("Hemorphite"), &CharacterData::ReprocessingSkills::mHemorphiteProcessing);
        insertSkillMapping(QStringLiteral("Ice"), &CharacterData::ReprocessingSkills::mIceProcessing);
        insertSkillMapping(QStringLiteral("Jaspet"), &CharacterData::ReprocessingSkills::mJaspetProcessing);
        insertSkillMapping(QStringLiteral("Kernite"), &CharacterData::ReprocessingSkills::mKerniteProcessing);
        insertSkillMapping(QStringLiteral("Mercoxit"), &CharacterData::ReprocessingSkills::mMercoxitProcessing);
        insertSkillMapping(QStringLiteral("Omber"), &CharacterData::ReprocessingSkills::mOmberProcessing);
        insertSkillMapping(QStringLiteral("Plagioclase"), &CharacterData::ReprocessingSkills::mPlagioclaseProcessing);
        insertSkillMapping(QStringLiteral("Pyroxeres"), &CharacterData::ReprocessingSkills::mPyroxeresProcessing);
        insertSkillMapping(QStringLiteral("Scordite"), &CharacterData::ReprocessingSkills::mScorditeProcessing);
        insertSkillMapping(QStringLiteral("Spodumain"), &CharacterData::ReprocessingSkills::mSpodumainProcessing);
        insertSkillMapping(QStringLiteral("Veldspar"), &CharacterData::ReprocessingSkills::mVeldsparProcessing);
    }

    int OreReprocessingArbitrageModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return numColumns;
    }

    QVariant OreReprocessingArbitrageModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return {};

        const auto column = index.column();
        const auto &data = mData[index.row()];

        switch (role) {
        case Qt::DisplayRole:
            switch (column) {
            case nameColumn:
                return mDataProvider.getTypeName(data.mId);
            }
            break;
        case Qt::UserRole:
            switch (column) {
            case nameColumn:
                return mDataProvider.getTypeName(data.mId);
            }
        }

        return {};
    }

    QVariant OreReprocessingArbitrageModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case nameColumn:
                return tr("Name");
            }
        }

        return {};
    }

    int OreReprocessingArbitrageModel::rowCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return static_cast<int>(mData.size());
    }

    EveType::IdType OreReprocessingArbitrageModel::getTypeId(const QModelIndex &index) const
    {
        if (!index.isValid())
            return EveType::invalidId;

        return mData[index.row()].mId;
    }

    void OreReprocessingArbitrageModel::setCharacter(std::shared_ptr<Character> character)
    {
        beginResetModel();
        mCharacter = std::move(character);
        endResetModel();
    }

    void OreReprocessingArbitrageModel::setOrderData(const std::vector<ExternalOrder> &orders,
                                                     PriceType dstPriceType,
                                                     const RegionList &srcRegions,
                                                     const RegionList &dstRegions,
                                                     quint64 srcStation,
                                                     quint64 dstStation,
                                                     bool useStationTax,
                                                     bool ignoreMinVolume,
                                                     double baseYield)
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        mData.clear();

        if (!mCharacter)
            return;

        const auto &reprocessingInfo = mDataProvider.getOreReprocessingInfo();
        const auto reprocessingSkills = mCharacter->getReprocessingSkills();
        const auto reprocessingYield = baseYield *
                                       (1 + reprocessingSkills.mReprocessing * 0.03) *
                                       (1 + reprocessingSkills.mReprocessingEfficiency * 0.02) *
                                       (1 + mCharacter->getReprocessingImplantBonus() / 100.);

        // gather src/dst orders for reprocessing types
        std::unordered_set<EveType::IdType> oreTypes, materialTypes;
        for (const auto &info : reprocessingInfo)
        {
            oreTypes.emplace(info.first);
            for (const auto &material : info.second.mMaterials)
                materialTypes.emplace(material.mMaterialId);
        }

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

        const auto isDstOrder = [&](const auto &order) {
            return order.getType() == dstPriceType &&
                   isValidRegion(allDstRegions, dstRegions, order) &&
                   isValidStation(dstStation, order);
        };

        const auto orderFilter = [&](const auto &order) {
            return (isSrcOrder(order) || isDstOrder(order)) && (!ignoreMinVolume || order.getMinVolume() <= 1);
        };

        std::unordered_map<EveType::IdType, std::multiset<ExternalOrder, ExternalOrder::LowToHigh>> sellMap;
        std::unordered_map<EveType::IdType, std::multiset<ExternalOrder, ExternalOrder::HighToLow>> buyMap;
        for (const auto &order : orders | boost::adaptors::filtered(orderFilter))
        {
            const auto typeId = order.getTypeId();
            if (oreTypes.find(order.getTypeId()) != std::end(oreTypes) && isSrcOrder(order))
                sellMap[typeId].emplace(order);
            if (materialTypes.find(order.getTypeId()) != std::end(materialTypes) && isDstOrder(order))
                buyMap[typeId].emplace(order);
        }

        // for given type, try to find arbitrage opportunities from source orders to dst orders
        // we have 2 versions to avoid branching logic - selling to buy orders and using sell orders

        struct UsedOrder
        {
            uint mVolume;
            double mPrice;
        };

        const auto buyVolume = [&](auto &orders, auto volume) {
            std::vector<UsedOrder> usedOrders;
            for (auto &order : orders)
            {
                const auto orderVolume = order.getVolumeRemaining();
                if (volume > order.getMinVolume() && orderVolume > 0)
                {
                    const auto amount = std::min(orderVolume, volume);
                    volume -= amount;

                    // NOTE: we're casting away const, but not modifying the actual set key
                    // looks dirty, but there's no partial constness
                    const_cast<ExternalOrder &>(order).setVolumeRemaining(orderVolume - amount);

                    UsedOrder used{amount, order.getPrice()};
                    usedOrders.emplace_back(used);
                }
            }

            return usedOrders;
        };

        // NOTE: using std::function because QtConcurrent::mapped cannot infer the result type properly
        const std::function<ItemData (const EveDataProvider::ReprocessingMap::value_type &)> findArbitrageForBuy = [&](const auto &reprocessingInfo) {
            const auto sellOrderList = sellMap.find(reprocessingInfo.first);
            if (sellOrderList == std::end(sellMap))
                return ItemData{};

            const auto skill = mReprocessingSkillMap.find(reprocessingInfo.first);
            if (skill == std::end(mReprocessingSkillMap))
            {
                qWarning() << "Missing reprocessing skill for" << reprocessingInfo.first;
                return ItemData{};
            }

            const auto requiredVolume = reprocessingInfo.second.mPortionSize;

            // keep buying and selling until no more orders are left or we stop making profit
            while (true)
            {
                // unsychronized access but that's ok, since only one thread touches given type orders
                const auto bought = buyVolume(sellOrderList->second, requiredVolume);
                if (bought.empty()) // no more volume to buy
                    break;

                auto cost = std::accumulate(std::begin(bought), std::end(bought), 0., [](auto total, const auto &order) {
                    return order.mVolume * order.mPrice + total;
                });

                // try to sell all the refined goods
                for (const auto &material : reprocessingInfo.second.mMaterials)
                {
                    const auto sellVolume = reprocessingYield * (1 + reprocessingSkills.*(skill->second) * 0.02) * material.mQuantity;

                }
            }

            ItemData data;
            data.mId = reprocessingInfo.first;

            return data;
        };

        // fill our destination collection
        const auto fillData = [](auto &result, const auto &itemData) {
            if (itemData.mId != EveType::invalidId)
                result.emplace_back(itemData);
        };

        // concurrently check for all arbitrage opportunities
        mData = QtConcurrent::blockingMappedReduced<decltype(mData)>(reprocessingInfo, findArbitrageForBuy, fillData);
    }

    void OreReprocessingArbitrageModel::reset()
    {
        beginResetModel();
        mData.clear();
        endResetModel();
    }

    void OreReprocessingArbitrageModel::insertSkillMapping(const QString &groupName, int CharacterData::ReprocessingSkills::* skill)
    {
        const auto groupId = mDataProvider.getGroupId(groupName);
        if (groupId == 0)
            throw std::runtime_error{"Cannot find group id for: " + groupName.toStdString()};

        mReprocessingSkillMap[groupId] = skill;
    }
}
