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
#include "Character.h"
#include "EveType.h"

namespace Evernus
{
    class AssetProvider;

    class IndustryManufacturingSetupModel
        : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        IndustryManufacturingSetupModel(IndustryManufacturingSetup &setup,
                                        const EveDataProvider &dataProvider,
                                        const AssetProvider &assetProvider,
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
        void refreshAssets();

        void setSource(EveType::IdType id, IndustryManufacturingSetup::InventorySource source);
        void setRuns(EveType::IdType id, uint runs);
        void setMaterialEfficiency(EveType::IdType id, uint value);
        void setTimeEfficiency(EveType::IdType id, uint value);

        void setCharacter(Character::IdType id);

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
            RunsRole,
            MaterialEfficiencyRole,
            TimeEfficiencyRole,
        };

        class TreeItem;

        using TreeItemPtr = std::unique_ptr<TreeItem>;

        class TreeItem final
        {
        public:
            TreeItem(IndustryManufacturingSetupModel &model,
                     const IndustryManufacturingSetup &setup);
            TreeItem(EveType::IdType typeId,
                     IndustryManufacturingSetupModel &model,
                     const IndustryManufacturingSetup &setup);
            ~TreeItem() = default;

            EveType::IdType getTypeId() const noexcept;

            uint getQuantityProduced() const noexcept;
            void setQuantityProduced(uint value) noexcept;

            uint getEffectiveQuantityRequired() const noexcept;
            uint getQuantityRequired() const noexcept;
            void setQuantityRequired(uint value) noexcept;

            uint getEffectiveRuns() const noexcept;
            uint getRuns() const noexcept;
            void setRuns(uint value) noexcept;

            std::chrono::seconds getTime() const noexcept;
            void setTime(std::chrono::seconds value) noexcept;

            TreeItem *getChild(int row) const;
            TreeItem *getParent() const noexcept;

            int getRow() const noexcept;

            int getChildCount() const noexcept;

            void appendChild(std::unique_ptr<TreeItem> child);
            void clearChildren() noexcept;

            inline auto begin() noexcept
            {
                return std::begin(mChildItems);
            }

            inline auto begin() const noexcept
            {
                return std::begin(mChildItems);
            }

            inline auto end() noexcept
            {
                return std::end(mChildItems);
            }

            inline auto end() const noexcept
            {
                return std::end(mChildItems);
            }

        private:
            IndustryManufacturingSetupModel &mModel;
            const IndustryManufacturingSetup &mSetup;
            TreeItem *mParent = nullptr;
            EveType::IdType mTypeId = EveType::invalidId;
            uint mQuantityProduced = 0;
            uint mQuantityRequired = 0;
            uint mRuns = 1;
            std::chrono::seconds mTime;
            std::vector<TreeItemPtr> mChildItems;
        };

        struct AssetQuantity
        {
            quint64 mInitialQuantity = 0;
            quint64 mCurrentQuantity = 0;
        };

        IndustryManufacturingSetup &mSetup;
        const EveDataProvider &mDataProvider;
        const AssetProvider &mAssetProvider;

        TreeItem mRoot{*this, mSetup};

        Character::IdType mCharacterId = Character::invalidId;

        std::unordered_multimap<EveType::IdType, std::reference_wrapper<TreeItem>> mTypeItemMap;

        std::unordered_map<EveType::IdType, AssetQuantity> mAssetQuantities;

        void fillChildren(TreeItem &item);

        TreeItemPtr createOutputItem(EveType::IdType typeId,
                                     const IndustryManufacturingSetup::OutputSettings &settings);
        TreeItemPtr createSourceItem(const EveDataProvider::MaterialInfo &materialInfo);

        template<class Iterator>
        void fillAssetList(Iterator begin, Iterator end);

        quint64 takeAssets(EveType::IdType typeId, quint64 max);

        void signalQuantityChange(EveType::IdType typeId);
        void roleAndQuantityChange(EveType::IdType typeId, const QVector<int> &roles);
    };
}
