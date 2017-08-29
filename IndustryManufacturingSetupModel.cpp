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
#include <boost/scope_exit.hpp>

#include "IndustryManufacturingSetupModel.h"

namespace Evernus
{
    IndustryManufacturingSetupModel::TreeItem::TreeItem(EveType::IdType typeId)
        : mTypeId{typeId}
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

    uint IndustryManufacturingSetupModel::TreeItem::getQuantityRequired() const noexcept
    {
        return mQuantityRequired;
    }

    void IndustryManufacturingSetupModel::TreeItem::setQuantityRequired(uint value) noexcept
    {
        mQuantityRequired = value;
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

    IndustryManufacturingSetupModel::IndustryManufacturingSetupModel(IndustryManufacturingSetup &setup,
                                                                     const EveDataProvider &dataProvider,
                                                                     QObject *parent)
        : QAbstractItemModel{parent}
        , mSetup{setup}
        , mDataProvider{dataProvider}
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
                return item->getQuantityRequired();
            case SourceRole:
                return static_cast<int>(mSetup.getTypeSettings(item->getTypeId()).mSource);
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

        const auto output = mSetup.getOutputTypes();
        for (const auto outputType : output)
        {
            auto child = createOutputItem(outputType);
            fillChildren(*child);

            mTypeItemMap.emplace(outputType, std::ref(*child));
            mRoot.appendChild(std::move(child));
        }
    }

    void IndustryManufacturingSetupModel::setSource(EveType::IdType id, IndustryManufacturingSetup::InventorySource source)
    {
        mSetup.setSource(id, source);

        const auto items = mTypeItemMap.equal_range(id);
        for (auto item = items.first; item != items.second; ++item)
        {
            const auto idx = createIndex(item->second.get().getRow(), 0, &item->second.get());
            emit dataChanged(idx, idx, { SourceRole });
        }
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

    IndustryManufacturingSetupModel::TreeItemPtr IndustryManufacturingSetupModel::createOutputItem(EveType::IdType typeId) const
    {
        const auto &manufacturingInfo = mSetup.getManufacturingInfo(typeId);

        auto item = std::make_unique<TreeItem>(typeId);
        item->setQuantityProduced(manufacturingInfo.mQuantity);

        return item;
    }

    IndustryManufacturingSetupModel::TreeItemPtr IndustryManufacturingSetupModel
    ::createSourceItem(const EveDataProvider::MaterialInfo &materialInfo) const
    {
        const auto &manufacturingInfo = mSetup.getManufacturingInfo(materialInfo.mMaterialId);

        auto item = std::make_unique<TreeItem>(materialInfo.mMaterialId);
        item->setQuantityProduced(manufacturingInfo.mQuantity);
        item->setQuantityRequired(materialInfo.mQuantity);

        return item;
    }
}
