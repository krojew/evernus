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
#include <future>
#include <cmath>

#include <boost/range/algorithm/max_element.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/scope_exit.hpp>

#include "ItemCostProvider.h"
#include "AssetProvider.h"
#include "AssetList.h"

#include "IndustryManufacturingSetupModel.h"

using namespace std::chrono_literals;

namespace Evernus
{
    IndustryManufacturingSetupModel::TreeItem::TreeItem(IndustryManufacturingSetupModel &model,
                                                        const IndustryManufacturingSetup &setup)
        : mModel{model}
        , mSetup{setup}
    {
    }

    IndustryManufacturingSetupModel::TreeItem::TreeItem(EveType::IdType typeId,
                                                        IndustryManufacturingSetupModel &model,
                                                        const IndustryManufacturingSetup &setup)
        : mModel{model}
        , mSetup{setup}
        , mTypeId{typeId}
        , mManufacturingInfo{mSetup.getManufacturingInfo(mTypeId)}
    {
    }

    EveType::IdType IndustryManufacturingSetupModel::TreeItem::getTypeId() const noexcept
    {
        return mTypeId;
    }

    quint64 IndustryManufacturingSetupModel::TreeItem::getEffectiveQuantityRequired() const
    {
        if (Q_UNLIKELY(isOutput()))
            return 0;

        const auto &settings = mSetup.getTypeSettings(mTypeId);
        if (settings.mSource == IndustryManufacturingSetup::InventorySource::Manufacture ||
            settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenManufacture)
        {
            return getEffectiveRuns() * mManufacturingInfo.mQuantity;
        }

        auto required = getQuantityRequiredForParent();
        if (settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenBuyAtCustomCost ||
            settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenBuyFromSource)
        {
            required -= mAssetQuantity;
        }

        return required;
    }

    uint IndustryManufacturingSetupModel::TreeItem::getQuantityRequired() const noexcept
    {
        return mQuantityRequired;
    }

    quint64 IndustryManufacturingSetupModel::TreeItem::getQuantityRequiredForParent() const
    {
        return IndustryUtils::getRequiredQuantity(mParent->getEffectiveRuns(),
                                                  mQuantityRequired,
                                                  mParent->getMaterialEfficiency(),
                                                  mModel.mFacilityType,
                                                  mModel.mSecurityStatus,
                                                  mModel.mMaterialRigType);
    }

    void IndustryManufacturingSetupModel::TreeItem::setQuantityRequired(uint value) noexcept
    {
        mQuantityRequired = value;
    }

    uint IndustryManufacturingSetupModel::TreeItem::getQuantityProduced() const noexcept
    {
        return mManufacturingInfo.mQuantity;
    }

    void IndustryManufacturingSetupModel::TreeItem::setAssetQuantity(quint64 value) noexcept
    {
        mAssetQuantity = value;
    }

    uint IndustryManufacturingSetupModel::TreeItem::getEffectiveRuns() const
    {
        if (Q_UNLIKELY(isOutput()))
            return mRuns;

        const auto &settings = mSetup.getTypeSettings(mTypeId);
        if (settings.mSource == IndustryManufacturingSetup::InventorySource::Manufacture ||
            settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenManufacture)
        {
            double required = getQuantityRequiredForParent();
            if (settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenManufacture)
                required -= mAssetQuantity;

            Q_ASSERT(mManufacturingInfo.mQuantity > 0);
            return std::ceil(required / mManufacturingInfo.mQuantity);
        }

        // we're buying this stuff, so no production
        return 0;
    }

    uint IndustryManufacturingSetupModel::TreeItem::getRuns() const noexcept
    {
        return mRuns;
    }

    void IndustryManufacturingSetupModel::TreeItem::setRuns(uint value) noexcept
    {
        mRuns = value;
    }

    std::chrono::seconds IndustryManufacturingSetupModel::TreeItem::getEffectiveTime() const
    {
        if (Q_UNLIKELY(isOutput()))
            return getTimeToManufacture();

        const auto &settings = mSetup.getTypeSettings(mTypeId);
        if (settings.mSource == IndustryManufacturingSetup::InventorySource::Manufacture ||
            settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenManufacture)
        {
            return getTimeToManufacture();
        }

        // we're buying this stuff
        return 0s;
    }

    std::chrono::seconds IndustryManufacturingSetupModel::TreeItem::getEffectiveTotalTime() const
    {
        const std::function<std::chrono::seconds (const TreeItemPtr &)> transform{[](const auto &child) {
            Q_ASSERT(child);
            return child->getEffectiveTotalTime();
        }};
        const auto maxChildTime = boost::max_element<boost::range_return_value::return_found_end>(
            mChildItems | boost::adaptors::transformed(transform)
        );
        const auto childTime = (maxChildTime.empty()) ? (0s) : (maxChildTime.front());

        return getEffectiveRuns() * getEffectiveTime() + childTime;
    }

    double IndustryManufacturingSetupModel::TreeItem::getCost() const
    {
        const auto childrenCost = std::accumulate(std::begin(mChildItems), std::end(mChildItems), 0., [](auto value, const auto &child) {
            return value + child->getCost();
        });

        double thisCost = 0.;

        // TODO: implement
        if (Q_UNLIKELY(isOutput()))
        {
        }
        else
        {
            const auto &settings = mSetup.getTypeSettings(mTypeId);
            switch (settings.mSource) {
            case IndustryManufacturingSetup::InventorySource::AcquireForFree:
                return 0.;
            case IndustryManufacturingSetup::InventorySource::BuyAtCustomCost:
            case IndustryManufacturingSetup::InventorySource::TakeAssetsThenBuyAtCustomCost:
                Q_ASSERT(mModel.mCharacter);
                thisCost = mModel.mCostProvider.fetchForCharacterAndType(mModel.mCharacter->getId(), mTypeId)->getAdjustedCost() *
                           getEffectiveQuantityRequired();
                break;
            case IndustryManufacturingSetup::InventorySource::BuyFromSource:
            case IndustryManufacturingSetup::InventorySource::TakeAssetsThenBuyFromSource:
                thisCost = mModel.getSrcPrice(mTypeId, getEffectiveQuantityRequired());
                break;
            case IndustryManufacturingSetup::InventorySource::Manufacture:
            case IndustryManufacturingSetup::InventorySource::TakeAssetsThenManufacture:
                thisCost = getManufacturingCost();
            }
        }

        return childrenCost + thisCost;
    }

    IndustryManufacturingSetupModel::TreeItem *IndustryManufacturingSetupModel::TreeItem::getChild(int row) const
    {
        return (row >= static_cast<int>(mChildItems.size())) ? (nullptr) : (mChildItems[row].get());
    }

    IndustryManufacturingSetupModel::TreeItem *IndustryManufacturingSetupModel::TreeItem::getParent() const noexcept
    {
        return mParent;
    }

    int IndustryManufacturingSetupModel::TreeItem::getRow() const noexcept
    {
        if (mParent != nullptr)
        {
            auto row = 0;
            for (const auto &child : mParent->mChildItems)
            {
                if (child.get() == this)
                    return row;

                ++row;
            }
        }

        return 0;
    }

    int IndustryManufacturingSetupModel::TreeItem::getChildCount() const noexcept
    {
        return static_cast<int>(mChildItems.size());
    }

    void IndustryManufacturingSetupModel::TreeItem::appendChild(std::unique_ptr<TreeItem> child)
    {
        child->mParent = this;
        mChildItems.emplace_back(std::move(child));
    }

    void IndustryManufacturingSetupModel::TreeItem::clearChildren() noexcept
    {
        mChildItems.clear();
    }

    uint IndustryManufacturingSetupModel::TreeItem::getMaterialEfficiency() const
    {
        if (Q_UNLIKELY(isOutput()))
        {
            const auto &settings = mSetup.getOutputSettings(mTypeId);
            return settings.mMaterialEfficiency;
        }

        const auto &settings = mSetup.getTypeSettings(mTypeId);
        return settings.mMaterialEfficiency;
    }

    uint IndustryManufacturingSetupModel::TreeItem::getTimeEfficiency() const
    {
        if (Q_UNLIKELY(isOutput()))
        {
            const auto &settings = mSetup.getOutputSettings(mTypeId);
            return settings.mTimeEfficiency;
        }

        const auto &settings = mSetup.getTypeSettings(mTypeId);
        return settings.mTimeEfficiency;
    }

    bool IndustryManufacturingSetupModel::TreeItem::isOutput() const
    {
        return mQuantityRequired == 0 || mParent == nullptr;
    }

    std::chrono::seconds IndustryManufacturingSetupModel::TreeItem::getTimeToManufacture() const
    {
        Q_ASSERT(mModel.mCharacter);

        auto skillModifier = (1.f - mModel.mCharacterManufacturingSkills[EveDataProvider::industrySkillId] * 4 / 100.f) *
                             (1.f - mModel.mCharacterManufacturingSkills[EveDataProvider::advancedIndustrySkillId] * 3 / 100.f);

        for (const auto skillId : mManufacturingInfo.mAdditionalsSkills)
            skillModifier *= (1.f - mModel.mCharacterManufacturingSkills[skillId] / 100.f);

        return IndustryUtils::getProductionTime(mManufacturingInfo.mTime,
                                                getTimeEfficiency(),
                                                mModel.mCharacter->getManufacturingTimeImplantBonus(),
                                                skillModifier,
                                                mModel.mFacilityType,
                                                mModel.mSecurityStatus,
                                                mModel.mFacilitySize,
                                                mModel.mTimeRigType);
    }

    double IndustryManufacturingSetupModel::TreeItem::getManufacturingCost() const
    {
        const auto baseJobCost = std::accumulate(
            std::begin(mManufacturingInfo.mMaterials),
            std::end(mManufacturingInfo.mMaterials),
            0.,
            [=](auto value, const auto &material) {
                return value + mModel.mMarketPrices[material.mMaterialId].mAdjustedPrice * material.mQuantity;
            }
        );

        const auto systemCostIndex = (mModel.mSrcSystemId == 0) ?
                                     (1.) :
                                     (mModel.mCostIndices[mModel.mSrcSystemId][IndustryCostIndex::Activity::Manufacturing]);

        const auto jobFee = baseJobCost * systemCostIndex * getEffectiveRuns();
        return jobFee * (1. + mModel.mFacilityTax / 100.);
    }

    IndustryManufacturingSetupModel::IndustryManufacturingSetupModel(IndustryManufacturingSetup &setup,
                                                                     const EveDataProvider &dataProvider,
                                                                     const AssetProvider &assetProvider,
                                                                     const ItemCostProvider &costProvider,
                                                                     const CharacterRepository &characterRepo,
                                                                     QObject *parent)
        : QAbstractItemModel{parent}
        , mSetup{setup}
        , mDataProvider{dataProvider}
        , mAssetProvider{assetProvider}
        , mCostProvider{costProvider}
        , mCharacterRepo{characterRepo}
    {
    }

    int IndustryManufacturingSetupModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return 1;
    }

    QVariant IndustryManufacturingSetupModel::data(const QModelIndex &index, int role) const
    {
        if (Q_UNLIKELY(!index.isValid()))
            return {};

        const auto item = static_cast<const TreeItem *>(index.internalPointer());
        Q_ASSERT(item != nullptr);

        try
        {
            switch (role) {
            case NameRole:
                return mDataProvider.getTypeName(item->getTypeId());
            case TypeIdRole:
                return item->getTypeId();
            case QuantityProducedRole:
                return item->getQuantityProduced();
            case QuantityRequiredRole:
                return item->getEffectiveQuantityRequired();
            case SourceRole:
                return static_cast<int>(mSetup.getTypeSettings(item->getTypeId()).mSource);
            case TimeRole:
                return formatDuration(item->getEffectiveTime());
            case RunsRole:
                return item->getRuns();
            case MaterialEfficiencyRole:
                {
                    const auto id = item->getTypeId();
                    if (item->getParent() == &mRoot)
                        return mSetup.getOutputSettings(id).mMaterialEfficiency;

                    return mSetup.getTypeSettings(id).mMaterialEfficiency;
                }
            case TimeEfficiencyRole:
                {
                    const auto id = item->getTypeId();
                    if (item->getParent() == &mRoot)
                        return mSetup.getOutputSettings(id).mTimeEfficiency;

                    return mSetup.getTypeSettings(id).mTimeEfficiency;
                }
            case TotalTimeRole:
                return formatDuration(item->getEffectiveTotalTime());
            case CostRole:
                return item->getCost();
            }
        }
        catch (const IndustryManufacturingSetup::NotSourceTypeException &e)
        {
            // ignore request for invalid items
            qDebug() << "Ignoring setup exception:" << e.what();
        }

        return {};
    }

    QModelIndex IndustryManufacturingSetupModel::index(int row, int column, const QModelIndex &parent) const
    {
        if (!hasIndex(row, column, parent))
             return {};

         const TreeItem *parentItem = nullptr;

         if (!parent.isValid())
             parentItem = &mRoot;
         else
             parentItem = static_cast<const TreeItem *>(parent.internalPointer());

         const auto childItem = parentItem->getChild(row);
         if (childItem)
             return createIndex(row, column, childItem);

         return {};
    }

    QModelIndex IndustryManufacturingSetupModel::parent(const QModelIndex &index) const
    {
        if (!index.isValid())
            return {};

        const auto childItem = static_cast<const TreeItem *>(index.internalPointer());
        const auto parentItem = childItem->getParent();

        if (parentItem == &mRoot)
            return {};

        return createIndex(parentItem->getRow(), 0, parentItem);
    }

    QHash<int, QByteArray> IndustryManufacturingSetupModel::roleNames() const
    {
        return {
            { NameRole, QByteArrayLiteral("name") },
            { TypeIdRole, QByteArrayLiteral("typeId") },
            { QuantityProducedRole, QByteArrayLiteral("quantityProduced") },
            { QuantityRequiredRole, QByteArrayLiteral("quantityRequired") },
            { SourceRole, QByteArrayLiteral("source") },
            { TimeRole, QByteArrayLiteral("time") },
            { RunsRole, QByteArrayLiteral("runs") },
            { MaterialEfficiencyRole, QByteArrayLiteral("materialEfficiency") },
            { TimeEfficiencyRole, QByteArrayLiteral("timeEfficiency") },
            { TotalTimeRole, QByteArrayLiteral("totalTime") },
            { CostRole, QByteArrayLiteral("cost") },
        };
    }

    int IndustryManufacturingSetupModel::rowCount(const QModelIndex &parent) const
    {
        const TreeItem *parentItem = nullptr;
         if (parent.column() > 0)
             return 0;

         if (!parent.isValid())
             parentItem = &mRoot;
         else
             parentItem = static_cast<const TreeItem *>(parent.internalPointer());

         return parentItem->getChildCount();
    }

    void IndustryManufacturingSetupModel::refreshData()
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        mRoot.clearChildren();
        mTypeItemMap.clear();

        const auto &output = mSetup.getOutputTypes();
        for (const auto &outputType : output)
        {
            auto child = createOutputItem(outputType.first, outputType.second);
            fillChildren(*child);

            mTypeItemMap.emplace(outputType.first, std::ref(*child));
            mRoot.appendChild(std::move(child));
        }

        if (mAssetQuantities.empty())
            refreshAssets();
        else
            fillItemAssets();
    }

    void IndustryManufacturingSetupModel::refreshAssets()
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        mAssetQuantities.clear();

        if (Q_LIKELY(mCharacter))
        {
            const auto assets = mAssetProvider.fetchAssetsForCharacter(mCharacter->getId());
            Q_ASSERT(assets);

            fillAssetList(std::begin(*assets), std::end(*assets));
        }

        fillItemAssets();
    }

    void IndustryManufacturingSetupModel
    ::setSource(EveType::IdType id, IndustryManufacturingSetup::InventorySource source)
    {
        mSetup.setSource(id, source);
        roleAndQuantityChange(id, { SourceRole, QuantityRequiredRole, TimeRole, TotalTimeRole, CostRole });
    }

    void IndustryManufacturingSetupModel::setRuns(EveType::IdType id, uint runs)
    {
        try
        {
            mSetup.setRuns(id, runs);

            const auto output = std::find_if(std::begin(mRoot), std::end(mRoot), [=](const auto &item) {
                Q_ASSERT(item);
                return item->getTypeId() == id;
            });
            if (Q_LIKELY(output != std::end(mRoot)))
            {
                const auto &item = *output;
                Q_ASSERT(item);

                item->setRuns(runs);
                fillItemAssets();

                // note: don't emit manufacturing roles change, because this will propagate from children

                for (const auto &child : *item)
                    signalManufacturingRolesChange(child->getTypeId());
            }
        }
        catch (const IndustryManufacturingSetup::NotOutputTypeException &)
        {
            qDebug() << "Ignoring setting of runs for non-output type:" << id;
        }
    }

    void IndustryManufacturingSetupModel::setMaterialEfficiency(EveType::IdType id, uint value)
    {
        mSetup.setMaterialEfficiency(id, value);
        roleAndQuantityChange(id, { QuantityRequiredRole, TotalTimeRole, CostRole }); // note: don't change efficiency role because of binding loop
    }

    void IndustryManufacturingSetupModel::setTimeEfficiency(EveType::IdType id, uint value)
    {
        mSetup.setTimeEfficiency(id, value);
        signalTimeChange(id);
    }

    void IndustryManufacturingSetupModel::setCharacter(Character::IdType id)
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        try
        {
            mCharacter = mCharacterRepo.find(id);
            Q_ASSERT(mCharacter);

            const auto industrySkills = mCharacter->getIndustrySkills();
            mCharacterManufacturingSkills[EveDataProvider::industrySkillId] = industrySkills.mIndustry;
            mCharacterManufacturingSkills[EveDataProvider::advancedIndustrySkillId] = industrySkills.mAdvancedIndustry;
            mCharacterManufacturingSkills[3398] = industrySkills.mAdvancedLargeShipConstruction;
            mCharacterManufacturingSkills[3397] = industrySkills.mAdvancedMediumShipConstruction;
            mCharacterManufacturingSkills[3395] = industrySkills.mAdvancedSmallShipConstruction;
            mCharacterManufacturingSkills[11444] = industrySkills.mAmarrStarshipEngineering;
            mCharacterManufacturingSkills[3396] = industrySkills.mAvancedIndustrialShipConstruction;
            mCharacterManufacturingSkills[11454] = industrySkills.mCaldariStarshipEngineering;
            mCharacterManufacturingSkills[11448] = industrySkills.mElectromagneticPhysics;
            mCharacterManufacturingSkills[11453] = industrySkills.mElectronicEngineering;
            mCharacterManufacturingSkills[11450] = industrySkills.mGallenteStarshipEngineering;
            mCharacterManufacturingSkills[11446] = industrySkills.mGravitonPhysics;
            mCharacterManufacturingSkills[11433] = industrySkills.mHighEnergyPhysics;
            mCharacterManufacturingSkills[11443] = industrySkills.mHydromagneticPhysics;
            mCharacterManufacturingSkills[11447] = industrySkills.mLaserPhysics;
            mCharacterManufacturingSkills[11452] = industrySkills.mMechanicalEngineering;
            mCharacterManufacturingSkills[11445] = industrySkills.mMinmatarStarshipEngineering;
            mCharacterManufacturingSkills[11529] = industrySkills.mMolecularEngineering;
            mCharacterManufacturingSkills[11451] = industrySkills.mNuclearPhysics;
            mCharacterManufacturingSkills[11441] = industrySkills.mPlasmaPhysics;
            mCharacterManufacturingSkills[11455] = industrySkills.mQuantumPhysics;
            mCharacterManufacturingSkills[11449] = industrySkills.mRocketScience;
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }

        mRoot.clearChildren();
        mTypeItemMap.clear();
        mAssetQuantities.clear();
    }

    void IndustryManufacturingSetupModel::setFacilityType(IndustryUtils::FacilityType type)
    {
        mFacilityType = type;
        signalManufacturingRolesChange();
    }

    void IndustryManufacturingSetupModel::setSecurityStatus(IndustryUtils::SecurityStatus status)
    {
        mSecurityStatus = status;
        signalManufacturingRolesChange();
    }

    void IndustryManufacturingSetupModel::setMaterialRigType(IndustryUtils::RigType type)
    {
        mMaterialRigType = type;
        signalManufacturingRolesChange();
    }

    void IndustryManufacturingSetupModel::setTimeRigType(IndustryUtils::RigType type)
    {
        mTimeRigType = type;
        signalRoleChange({ TimeRole, TotalTimeRole });
    }

    void IndustryManufacturingSetupModel::setFacilitySize(IndustryUtils::Size size)
    {
        mFacilitySize = size;
        signalTimeChange();
    }

    void IndustryManufacturingSetupModel::setPriceTypes(PriceType src, PriceType dst)
    {
        mSrcPrice = src;
        mDstPrice = dst;

        signalRoleChange({ CostRole });
    }

    void IndustryManufacturingSetupModel::setFacilityTax(double value)
    {
        mFacilityTax = value;
        signalRoleChange({ CostRole });
    }

    void IndustryManufacturingSetupModel::setOrders(const std::vector<ExternalOrder> &orders,
                                                    const RegionList &srcRegions,
                                                    const RegionList &dstRegions,
                                                    quint64 srcStation,
                                                    quint64 dstStation)
    {
        mSrcSellOrders.clear();
        mDstBuyOrders.clear();

        mSrcSystemId = mDataProvider.getStationSolarSystemId(srcStation);

        const auto srcRegionId = (srcStation == 0) ? (0u) : (mDataProvider.getStationRegionId(srcStation));
        const auto dstRegionId = (dstStation == 0) ? (0u) : (mDataProvider.getStationRegionId(dstStation));

        const auto srcOrderFilter = [=, &srcRegions](const auto &order) {
            const auto regionId = order.getRegionId();
            if (srcRegions.find(regionId) == std::end(srcRegions))
                return false;

            const auto stationId = order.getStationId();
            return srcRegionId == 0 || srcRegionId != regionId || stationId == srcStation;
        };
        const auto dstOrderFilter = [=, &dstRegions](const auto &order) {
            const auto regionId = order.getRegionId();
            if (dstRegions.find(regionId) == std::end(dstRegions))
                return false;

            const auto stationId = order.getStationId();
            return dstRegionId == 0 || dstRegionId != regionId || stationId == dstStation;
        };

        auto srcFuture = std::async(std::launch::async, [&, orders = orders | boost::adaptors::filtered(srcOrderFilter)] {
            for (const auto &order : orders)
            {
                const auto typeId = order.getTypeId();

                if (order.getType() == ExternalOrder::Type::Buy)
                {
                    const auto price = order.getPrice();

                    if (price > mSrcBuyPrices[typeId])
                        mSrcBuyPrices[typeId] = price;
                }
                else
                {
                    mSrcSellOrders[typeId].emplace(order);
                }
            }
        });
        auto dstFuture = std::async(std::launch::async, [&, orders = orders | boost::adaptors::filtered(dstOrderFilter)] {
            for (const auto &order : orders)
            {
                const auto typeId = order.getTypeId();

                if (order.getType() == ExternalOrder::Type::Sell)
                {
                    const auto price = order.getPrice();

                    if (mDstSellPrices[typeId] == 0. || price < mDstSellPrices[typeId])
                        mDstSellPrices[typeId] = price;
                }
                else
                {
                    mDstBuyOrders[typeId].emplace(order);
                }
            }
        });

        srcFuture.get();
        dstFuture.get();

        signalRoleChange({ CostRole });
    }

    void IndustryManufacturingSetupModel::setMarketPrices(MarketPrices prices)
    {
        mMarketPrices = std::move(prices);
        signalRoleChange({ CostRole });
    }

    void IndustryManufacturingSetupModel::setCostIndices(IndustryCostIndices indices)
    {
        mCostIndices = std::move(indices);
        signalRoleChange({ CostRole });
    }

    void IndustryManufacturingSetupModel::fillChildren(TreeItem &item)
    {
        const auto &info = mSetup.getManufacturingInfo(item.getTypeId());
        for (const auto &source : info.mMaterials)
        {
            auto child = createSourceItem(source);
            fillChildren(*child);

            mTypeItemMap.emplace(source.mMaterialId, std::ref(*child));
            item.appendChild(std::move(child));
        }
    }

    IndustryManufacturingSetupModel::TreeItemPtr IndustryManufacturingSetupModel
    ::createOutputItem(EveType::IdType typeId, const IndustryManufacturingSetup::OutputSettings &settings)
    {
        auto item = std::make_unique<TreeItem>(typeId, *this, mSetup);
        item->setRuns(settings.mRuns);

        return item;
    }

    IndustryManufacturingSetupModel::TreeItemPtr IndustryManufacturingSetupModel
    ::createSourceItem(const EveDataProvider::MaterialInfo &materialInfo)
    {
        auto item = std::make_unique<TreeItem>(materialInfo.mMaterialId, *this, mSetup);
        item->setQuantityRequired(materialInfo.mQuantity);

        return item;
    }

    template<class Iterator>
    void IndustryManufacturingSetupModel::fillAssetList(Iterator begin, Iterator end)
    {
        while (begin != end)
        {
            const auto &item = *begin;

            mAssetQuantities[item->getTypeId()].mInitialQuantity += item->getQuantity();
            mAssetQuantities[item->getTypeId()].mCurrentQuantity += item->getQuantity();

            fillAssetList(std::begin(*item), std::end(*item));

            ++begin;
        }
    }

    quint64 IndustryManufacturingSetupModel::takeAssets(EveType::IdType typeId, quint64 max)
    {
        auto &quantities = mAssetQuantities[typeId];

        const auto quantityTaken = std::min(max, quantities.mCurrentQuantity);
        quantities.mCurrentQuantity -= quantityTaken;

        return quantityTaken;
    }

    void IndustryManufacturingSetupModel::fillItemAssets()
    {
        for (auto &asset : mAssetQuantities)
            asset.second.mCurrentQuantity = asset.second.mInitialQuantity;

        for (const auto &item : mTypeItemMap)
        {
            auto &realItem = item.second.get();
            if (realItem.getParent() == &mRoot || mAssetQuantities[item.first].mCurrentQuantity == 0)
                continue;

            const auto &settings = mSetup.getTypeSettings(realItem.getTypeId());
            if (settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenBuyAtCustomCost ||
                settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenBuyFromSource ||
                settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenManufacture)
            {
                realItem.setAssetQuantity(takeAssets(item.first, realItem.getQuantityRequiredForParent()));
            }
        }
    }

    void IndustryManufacturingSetupModel::signalQuantityChange(EveType::IdType typeId)
    {
        signalRoleChange(typeId, { RunsRole, QuantityRequiredRole, TotalTimeRole, CostRole });
    }

    void IndustryManufacturingSetupModel::signalTimeChange(EveType::IdType typeId)
    {
        signalRoleChange(typeId, { TimeRole, TotalTimeRole });
    }

    void IndustryManufacturingSetupModel::signalRoleChange(const QVector<int> &roles)
    {
        for (const auto &item : mTypeItemMap)
            signalRoleChange(item.second.get(), roles);
    }

    void IndustryManufacturingSetupModel::signalRoleChange(EveType::IdType typeId, const QVector<int> &roles)
    {
        std::unordered_set<EveType::IdType> remaining{typeId}, inspected;

        auto parentDependentRolesChanged =
            roles.contains(TimeEfficiencyRole) ||
            roles.contains(QuantityRequiredRole) ||
            roles.contains(SourceRole) ||
            roles.contains(CostRole) ||
            roles.contains(TimeRole) ||
            roles.contains(TotalTimeRole);

        do {
            const auto nextId = *std::begin(remaining);
            remaining.erase(std::begin(remaining));

            Q_ASSERT(inspected.find(nextId) == std::end(inspected));
            inspected.emplace(nextId);

            const auto &manufacturingInfo = mSetup.getManufacturingInfo(nextId);
            for (const auto &source : manufacturingInfo.mMaterials)
            {
                if (inspected.find(source.mMaterialId) == std::end(inspected))
                    remaining.insert(source.mMaterialId);
            }

            const auto items = mTypeItemMap.equal_range(nextId);
            for (auto item = items.first; item != items.second; ++item)
            {
                signalRoleChange(item->second.get(), roles);
                if (parentDependentRolesChanged)
                    signalParentDependentRolesChange(item->second.get());
            }

            // we've notified parents of the original type ids, so now we're only notifying our children
            parentDependentRolesChanged = false;
        } while (!remaining.empty());
    }

    void IndustryManufacturingSetupModel::signalTimeChange()
    {
        signalRoleChange({ TimeRole, TotalTimeRole });
    }

    void IndustryManufacturingSetupModel::signalParentDependentRolesChange(const TreeItem &item)
    {
        auto parent = item.getParent();
        while (parent != nullptr && parent != &mRoot)
        {
            signalRoleChange(*parent, { TotalTimeRole, CostRole });
            parent = parent->getParent();
        }
    }

    void IndustryManufacturingSetupModel::signalManufacturingRolesChange()
    {
        fillItemAssets();
        signalRoleChange({ RunsRole, QuantityRequiredRole, TimeRole, TotalTimeRole, CostRole });
    }

    void IndustryManufacturingSetupModel::signalManufacturingRolesChange(EveType::IdType typeId)
    {
        signalRoleChange(typeId, { RunsRole, QuantityRequiredRole, TimeRole, TotalTimeRole, CostRole });
    }

    void IndustryManufacturingSetupModel::signalRoleChange(TreeItem &item, const QVector<int> &roles)
    {
        const auto idx = createIndex(item.getRow(), 0, &item);
        emit dataChanged(idx, idx, roles);
    }

    void IndustryManufacturingSetupModel::roleAndQuantityChange(EveType::IdType typeId, const QVector<int> &roles)
    {
        fillItemAssets();
        signalRoleChange(typeId, roles);
    }

    double IndustryManufacturingSetupModel::getSrcPrice(EveType::IdType typeId, quint64 quantity) const
    {
        return (mSrcPrice == PriceType::Buy) ? (getSrcBuyPrice(typeId, quantity)) : (getSrcSellPrice(typeId, quantity));
    }

    double IndustryManufacturingSetupModel::getSrcBuyPrice(EveType::IdType typeId, quint64 quantity) const
    {
        const auto orders = mSrcBuyPrices.find(typeId);
        return quantity * ((orders == std::end(mSrcBuyPrices)) ? (0.) : (orders->second + 0.01));
    }

    double IndustryManufacturingSetupModel::getSrcSellPrice(EveType::IdType typeId, quint64 quantity) const
    {
        const auto orders = mSrcSellOrders.find(typeId);
        return (orders == std::end(mSrcSellOrders)) ? (0.) : (getPriceFromOrderList(orders->second, quantity));
    }

    double IndustryManufacturingSetupModel::getDstPrice(EveType::IdType typeId, quint64 quantity) const
    {
        return (mDstPrice == PriceType::Buy) ? (getDstBuyPrice(typeId, quantity)) : (getDstSellPrice(typeId, quantity));
    }

    double IndustryManufacturingSetupModel::getDstBuyPrice(EveType::IdType typeId, quint64 quantity) const
    {
        const auto orders = mDstBuyOrders.find(typeId);
        return (orders == std::end(mDstBuyOrders)) ? (0.) : (getPriceFromOrderList(orders->second, quantity));
    }

    double IndustryManufacturingSetupModel::getDstSellPrice(EveType::IdType typeId, quint64 quantity) const
    {
        const auto orders = mDstSellPrices.find(typeId);
        return quantity * ((orders == std::end(mDstSellPrices)) ? (0.) : (orders->second - 0.01));
    }

    template<class T>
    double IndustryManufacturingSetupModel::getPriceFromOrderList(const T &orders, quint64 quantity)
    {
        auto order = std::begin(orders);
        auto price = 0.;

        while (order != std::end(orders) && quantity > 0)
        {
            const auto amount = std::min(quantity, static_cast<quint64>(order->getVolumeRemaining()));

            price += amount * order->getPrice();
            quantity -= amount;
            ++order;
        }

        if (quantity > 0)
        {
            // not enough order to fulfill - estimate from best order
            return (Q_UNLIKELY(orders.empty())) ? (0.) : (price + std::begin(orders)->getPrice() * quantity);
        }

        return price;
    }

    QString IndustryManufacturingSetupModel::formatDuration(std::chrono::seconds time)
    {
        const qulonglong timeCount = time.count();
        return QStringLiteral("%1:%2:%3")
            .arg(timeCount / 3600, 2, 10, QLatin1Char('0'))
            .arg((timeCount / 60) % 60, 2, 10, QLatin1Char('0'))
            .arg(timeCount % 60, 2, 10, QLatin1Char('0'));
    }
}
