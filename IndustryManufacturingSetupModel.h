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
#pragma once

#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include <chrono>

#include <QAbstractItemModel>

#include "IndustryManufacturingSetup.h"
#include "EveDataProvider.h"
#include "EveType.h"

namespace Evernus
{
    class IndustryManufacturingSetupModel
        : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        IndustryManufacturingSetupModel(IndustryManufacturingSetup &setup,
                                        const EveDataProvider &dataProvider,
                                        QObject *parent = nullptr);
        IndustryManufacturingSetupModel(const IndustryManufacturingSetupModel &) = default;
        IndustryManufacturingSetupModel(IndustryManufacturingSetupModel &&) = default;
        virtual ~IndustryManufacturingSetupModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex{}) const override;
        virtual QModelIndex parent(const QModelIndex &index) const override;
        virtual QHash<int, QByteArray> roleNames() const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        void refreshData();

        void setSource(EveType::IdType id, IndustryManufacturingSetup::InventorySource source);

        IndustryManufacturingSetupModel &operator =(const IndustryManufacturingSetupModel &) = default;
        IndustryManufacturingSetupModel &operator =(IndustryManufacturingSetupModel &&) = default;

    private:
        enum
        {
            NameRole = Qt::UserRole,
            TypeIdRole,
            QuantityProducedRole,
            QuantityRequiredRole,
            SourceRole,
            TimeRole,
        };

        class TreeItem;

        using TreeItemPtr = std::unique_ptr<TreeItem>;

        class TreeItem final
        {
        public:
            TreeItem() = default;
            explicit TreeItem(EveType::IdType typeId);
            ~TreeItem() = default;

            EveType::IdType getTypeId() const noexcept;

            uint getQuantityProduced() const noexcept;
            void setQuantityProduced(uint value) noexcept;

            uint getQuantityRequired() const noexcept;
            void setQuantityRequired(uint value) noexcept;

            std::chrono::seconds getTime() const noexcept;
            void setTime(std::chrono::seconds value) noexcept;

            TreeItem *getChild(int row) const;
            TreeItem *getParent() const noexcept;

            int getRow() const noexcept;

            int getChildCount() const noexcept;

            void appendChild(std::unique_ptr<TreeItem> child);
            void clearChildren() noexcept;

        private:
            TreeItem *mParent = nullptr;
            EveType::IdType mTypeId = EveType::invalidId;
            uint mQuantityProduced = 0;
            uint mQuantityRequired = 0;
            std::chrono::seconds mTime;
            std::vector<TreeItemPtr> mChildItems;
        };

        IndustryManufacturingSetup &mSetup;
        const EveDataProvider &mDataProvider;

        TreeItem mRoot;

        std::unordered_multimap<EveType::IdType, std::reference_wrapper<TreeItem>> mTypeItemMap;

        void fillChildren(TreeItem &item);

        TreeItemPtr createOutputItem(EveType::IdType typeId) const;
        TreeItemPtr createSourceItem(const EveDataProvider::MaterialInfo &materialInfo) const;
    };
}
