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
#include <cmath>

#include <boost/scope_exit.hpp>

#include "IndustryUtils.h"
#include "AssetProvider.h"
#include "AssetList.h"

#include "IndustryManufacturingSetupModel.h"

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
    {
    }

    EveType::IdType IndustryManufacturingSetupModel::TreeItem::getTypeId() const noexcept
    {
        return mTypeId;
    }

    uint IndustryManufacturingSetupModel::TreeItem::getQuantityProduced() const noexcept
    {
        return mQuantityProduced;
    }

    void IndustryManufacturingSetupModel::TreeItem::setQuantityProduced(uint value) noexcept
    {
        mQuantityProduced = value;
    }

    uint IndustryManufacturingSetupModel::TreeItem::getEffectiveQuantityRequired() const noexcept
    {
        if (Q_UNLIKELY(isOutput()))
            return 0;

        const auto &settings = mSetup.getTypeSettings(mTypeId);
        if (settings.mSource == IndustryManufacturingSetup::InventorySource::Manufacture ||
            settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenManufacture)
        {
            return getEffectiveRuns() * mQuantityProduced;
        }

        auto required = mParent->getEffectiveRuns() * mQuantityRequired;
        if (settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenBuyAtCustomCost ||
            settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenBuyFromSource)
        {
            required -= mModel.takeAssets(mTypeId, required);
        }

        return required;
    }

    uint IndustryManufacturingSetupModel::TreeItem::getQuantityRequired() const noexcept
    {
        return mQuantityRequired;
    }

    void IndustryManufacturingSetupModel::TreeItem::setQuantityRequired(uint value) noexcept
    {
        mQuantityRequired = value;
    }

    uint IndustryManufacturingSetupModel::TreeItem::getEffectiveRuns() const noexcept
    {
        if (Q_UNLIKELY(isOutput()))
            return mRuns;

        const auto &settings = mSetup.getTypeSettings(mTypeId);
        if (settings.mSource == IndustryManufacturingSetup::InventorySource::Manufacture ||
            settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenManufacture)
        {
            auto required = IndustryUtils::getRequiredQuantity(mParent->getEffectiveRuns(),
                                                               mQuantityRequired,
                                                               mParent->getMaterialEfficiency());
            if (settings.mSource == IndustryManufacturingSetup::InventorySource::TakeAssetsThenManufacture)
                required -= mModel.takeAssets(mTypeId, required);

            Q_ASSERT(mQuantityProduced > 0);
            return std::ceil(required / mQuantityProduced);
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

    std::chrono::seconds IndustryManufacturingSetupModel::TreeItem::getTime() const noexcept
    {
        return mTime;
    }

    void IndustryManufacturingSetupModel::TreeItem::setTime(std::chrono::seconds value) noexcept
    {
        mTime = value;
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

    bool IndustryManufacturingSetupModel::TreeItem::isOutput() const
    {
        return mQuantityRequired == 0 || mParent == nullptr;
    }

    IndustryManufacturingSetupModel::IndustryManufacturingSetupModel(IndustryManufacturingSetup &setup,
                                                                     const EveDataProvider &dataProvider,
                                                                     const AssetProvider &assetProvider,
                                                                     QObject *parent)
        : QAbstractItemModel{parent}
        , mSetup{setup}
        , mDataProvider{dataProvider}
        , mAssetProvider{assetProvider}
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
                {
                    const qulonglong time = item->getTime().count();
                    return QStringLiteral("%1:%2:%3").arg(time / 3600, 2, 10, QLatin1Char('0')).arg((time / 60) % 60, 2, 10, QLatin1Char('0')).arg(time % 60, 2, 10, QLatin1Char('0'));
                }
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

        if (mAssetQuantities.empty())
        {
            refreshAssets();
        }
        else
        {
            for (auto &asset : mAssetQuantities)
                asset.second.mCurrentQuantity = asset.second.mInitialQuantity;
        }

        const auto &output = mSetup.getOutputTypes();
        for (const auto &outputType : output)
        {
            auto child = createOutputItem(outputType.first, outputType.second);
            fillChildren(*child);

            mTypeItemMap.emplace(outputType.first, std::ref(*child));
            mRoot.appendChild(std::move(child));
        }
    }

    void IndustryManufacturingSetupModel::refreshAssets()
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        mAssetQuantities.clear();

        const auto assets = mAssetProvider.fetchAssetsForCharacter(mCharacterId);
        Q_ASSERT(assets);

        fillAssetList(std::begin(*assets), std::end(*assets));
    }

    void IndustryManufacturingSetupModel
    ::setSource(EveType::IdType id, IndustryManufacturingSetup::InventorySource source)
    {
        mSetup.setSource(id, source);
        mAssetQuantities[id].mCurrentQuantity = mAssetQuantities[id].mInitialQuantity;

        roleAndQuantityChange(id, { SourceRole, QuantityRequiredRole });
    }

    void IndustryManufacturingSetupModel::setRuns(EveType::IdType id, uint runs)
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

            const auto idx = createIndex(item->getRow(), 0, item.get());
            emit dataChanged(idx, idx, { RunsRole });

            for (const auto &child : *item)
                signalQuantityChange(child->getTypeId());
        }
    }

    void IndustryManufacturingSetupModel::setMaterialEfficiency(EveType::IdType id, uint value)
    {
        mSetup.setMaterialEfficiency(id, value);
        roleAndQuantityChange(id, { QuantityRequiredRole }); // note: don't change efficiency role because of binding loop
    }

    void IndustryManufacturingSetupModel::setTimeEfficiency(EveType::IdType id, uint value)
    {
        mSetup.setTimeEfficiency(id, value);
        roleAndQuantityChange(id, { QuantityRequiredRole }); // note: don't change efficiency role because of binding loop
    }

    void IndustryManufacturingSetupModel::setCharacter(Character::IdType id)
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        mCharacterId = id;

        mRoot.clearChildren();
        mTypeItemMap.clear();
        mAssetQuantities.clear();
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
        const auto &manufacturingInfo = mSetup.getManufacturingInfo(typeId);

        auto item = std::make_unique<TreeItem>(typeId, *this, mSetup);
        item->setQuantityProduced(manufacturingInfo.mQuantity);
        item->setRuns(settings.mRuns);
        item->setTime(manufacturingInfo.mTime);

        return item;
    }

    IndustryManufacturingSetupModel::TreeItemPtr IndustryManufacturingSetupModel
    ::createSourceItem(const EveDataProvider::MaterialInfo &materialInfo)
    {
        const auto &manufacturingInfo = mSetup.getManufacturingInfo(materialInfo.mMaterialId);

        auto item = std::make_unique<TreeItem>(materialInfo.mMaterialId, *this, mSetup);
        item->setQuantityProduced(manufacturingInfo.mQuantity);
        item->setQuantityRequired(materialInfo.mQuantity);
        item->setTime(manufacturingInfo.mTime);

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

    void IndustryManufacturingSetupModel::signalQuantityChange(EveType::IdType typeId)
    {
        std::unordered_set<EveType::IdType> remaining{typeId}, inspected;

        do {
            const auto nextId = *std::begin(remaining);
            remaining.erase(std::begin(remaining));

            Q_ASSERT(inspected.find(nextId) == std::end(inspected));
            inspected.emplace(nextId);

            auto &quantities = mAssetQuantities[nextId];
            quantities.mCurrentQuantity = quantities.mInitialQuantity;

            const auto &manufacturingInfo = mSetup.getManufacturingInfo(nextId);
            for (const auto &source : manufacturingInfo.mMaterials)
            {
                if (inspected.find(source.mMaterialId) == std::end(inspected))
                    remaining.insert(source.mMaterialId);
            }

            const auto items = mTypeItemMap.equal_range(nextId);
            for (auto item = items.first; item != items.second; ++item)
            {
                const auto idx = createIndex(item->second.get().getRow(), 0, &item->second.get());
                emit dataChanged(idx, idx, { RunsRole, QuantityRequiredRole });
            }
        } while (!remaining.empty());
    }

    void IndustryManufacturingSetupModel::roleAndQuantityChange(EveType::IdType typeId, const QVector<int> &roles)
    {
        const auto items = mTypeItemMap.equal_range(typeId);
        for (auto item = items.first; item != items.second; ++item)
        {
            const auto idx = createIndex(item->second.get().getRow(), 0, &item->second.get());
            emit dataChanged(idx, idx, roles);

            for (const auto &child : item->second.get())
            {
                Q_ASSERT(child);
                signalQuantityChange(child->getTypeId());
            }
        }
    }
}
