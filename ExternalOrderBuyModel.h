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

#include <unordered_set>
#include <vector>
#include <memory>
#include <limits>

#include "ExternalOrderModel.h"
#include "ExternalOrder.h"
#include "MarketOrder.h"
#include "Character.h"

namespace Evernus
{
    class ExternalOrderRepository;
    class CharacterRepository;
    class MarketOrderProvider;
    class ItemCostProvider;
    class EveDataProvider;

    class ExternalOrderBuyModel
        : public ExternalOrderModel
    {
        Q_OBJECT

    public:
        ExternalOrderBuyModel(const EveDataProvider &dataProvider,
                               const ExternalOrderRepository &orderRepo,
                               const CharacterRepository &characterRepo,
                               const MarketOrderProvider &orderProvider,
                               const MarketOrderProvider &corpOrderProvider,
                               const ItemCostProvider &costProvider,
                               QObject *parent = nullptr);
        virtual ~ExternalOrderBuyModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex{}) const override;
        virtual QModelIndex parent(const QModelIndex &index) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        virtual int getPriceColumn() const override;
        virtual Qt::SortOrder getPriceSortOrder() const override;
        virtual int getVolumeColumn() const override;

        virtual uint getTotalVolume() const override;
        virtual double getTotalSize() const override;
        virtual double getTotalPrice() const override;
        virtual double getMedianPrice() const override;
        virtual double getMaxPrice() const override;
        virtual double getMinPrice() const override;

        virtual const ExternalOrder &getOrder(size_t row) const override;

        void setCharacter(Character::IdType id);
        void setRegionId(uint id);
        void setSolarSystemId(uint id);
        void setStationId(uint id);

        EveType::IdType getTypeId() const noexcept;
        void setTypeId(EveType::IdType id) noexcept;

        void reset();

        void changeDeviationSource(DeviationSourceType type, double value);

        void setGrouping(Grouping grouping);

    private:
        static const auto stationColumn = 0;
        static const auto deviationColumn = 1;
        static const auto priceColumn = 2;
        static const auto totalCostColumn = 3;
        static const auto volumeColumn = 4;
        static const auto rangeColumn = 5;
        static const auto minQunatityColumn = 6;
        static const auto totalSizeColumn = 7;
        static const auto issuedColumn = 8;
        static const auto durationColumn = 9;
        static const auto updatedColumn = 10;

        static const auto groupByColumn = 0;
        static const auto lowestPriceColumn = 1;
        static const auto medianPriceColumn = 2;
        static const auto highestPriceColumn = 3;
        static const auto groupedTotalCostColumn = 5;
        static const auto ordersColumn = 6;
        static const auto groupedTotalSizeColumn = 7;

        struct GroupedData
        {
            uint mId = 0;
            double mLowestPrice = std::numeric_limits<double>::max();
            double mMedianPrice = 0.;
            double mHighestPrice = 0.;
            uint mVolumeEntered = 0;
            uint mVolumeRemaining = 0;
            double mTotalCost = 0.;
            uint mCount = 0;
            double mTotalSize = 0.;
        };

        const EveDataProvider &mDataProvider;
        const ExternalOrderRepository &mOrderRepo;
        const CharacterRepository &mCharacterRepo;
        const MarketOrderProvider &mOrderProvider;
        const MarketOrderProvider &mCorpOrderProvider;
        const ItemCostProvider &mCostProvider;

        Character::IdType mCharacterId = Character::invalidId;

        EveType::IdType mTypeId = EveType::invalidId;
        uint mRegionId = 0, mSolarSystemId = 0, mStationId = 0;

        DeviationSourceType mDeviationType = DeviationSourceType::Median;
        double mDeviationValue = 1.;

        double mTotalPrice = 0., mMedianPrice = 0., mMinPrice = 0., mMaxPrice = 0.;
        double mTotalSize = 0.;
        uint mTotalVolume = 0;

        std::vector<std::shared_ptr<ExternalOrder>> mOrders;
        std::vector<GroupedData> mGroupedData;

        std::unordered_set<MarketOrder::IdType> mOwnOrders;

        Grouping mGrouping = Grouping::None;

        double computeDeviation(const ExternalOrder &order) const;

        void refreshGroupedData();

        QVariant getUngroupedData(int column, int role, const ExternalOrder &order) const;
        QVariant getStationGroupedData(int column, int role, const GroupedData &data) const;
        QVariant getSystemGroupedData(int column, int role, const GroupedData &data) const;
        QVariant getRegionGroupedData(int column, int role, const GroupedData &data) const;
        QVariant getGenericGroupedData(int column, int role, const GroupedData &data) const;

        template<uint (ExternalOrder::* Func)() const>
        void fillGroupedData();
    };
}
