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

#include <vector>
#include <memory>

#include <QAbstractItemModel>

#include "EveDataProvider.h"
#include "EveType.h"

namespace Evernus
{
    class IndustryManufacturingSetup;

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

        IndustryManufacturingSetupModel &operator =(const IndustryManufacturingSetupModel &) = default;
        IndustryManufacturingSetupModel &operator =(IndustryManufacturingSetupModel &&) = default;

    private:
        enum
        {
            NameRole = Qt::UserRole,
            TypeIdRole
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

            TreeItem *getChild(int row) const;
            TreeItem *getParent() const noexcept;

            int getRow() const noexcept;

            int getChildCount() const noexcept;

            void appendChild(std::unique_ptr<TreeItem> child);
            void clearChildren() noexcept;

        private:
            TreeItem *mParent = nullptr;
            EveType::IdType mTypeId = EveType::invalidId;
            std::vector<TreeItemPtr> mChildItems;
        };

        IndustryManufacturingSetup &mSetup;
        const EveDataProvider &mDataProvider;

        TreeItem mRoot;

        void fillChildren(TreeItem &item) const;

        static TreeItemPtr createOutputItem(EveType::IdType typeId);
        static TreeItemPtr createSourceItem(const EveDataProvider::MaterialInfo &info);
    };
}
