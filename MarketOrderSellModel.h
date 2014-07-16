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

#include "MarketOrderModel.h"
#include "MarketOrder.h"
#include "Character.h"

namespace Evernus
{
    class MarketOrderProvider;
    class ItemCostProvider;
    class EveDataProvider;
    class MarketOrder;

    class MarketOrderSellModel
        : public MarketOrderModel
    {
    public:
        explicit MarketOrderSellModel(const MarketOrderProvider &orderProvider,
                                      const EveDataProvider &dataProvider,
                                      const ItemCostProvider &itemCostProvider,
                                      QObject *parent = nullptr);
        virtual ~MarketOrderSellModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex{}) const override;
        virtual QModelIndex parent(const QModelIndex &index) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        virtual size_t getOrderCount() const override;
        virtual quint64 getVolumeRemaining() const override;
        virtual quint64 getVolumeEntered() const override;
        virtual double getTotalISK() const override;
        virtual double getTotalSize() const override;

        void setCharacter(Character::IdType id);

        void reset();

    private:
        static const auto nameColumn = 0;
        static const auto groupColumn = 1;
        static const auto statusColumn = 2;
        static const auto customCostColumn = 3;
        static const auto priceColumn = 4;
        static const auto priceStatusColumn = 5;
        static const auto volumeColumn = 6;
        static const auto totalColumn = 7;
        static const auto deltaColumn = 8;
        static const auto profitColumn = 9;
        static const auto totalProfitColumn = 10;
        static const auto profitPerItemColumn = 11;
        static const auto timeLeftColumn = 12;
        static const auto orderAgeColumn = 13;
        static const auto firstSeenColumn = 14;
        static const auto stationColumn = 15;

        const MarketOrderProvider &mOrderProvider;
        const EveDataProvider &mDataProvider;
        const ItemCostProvider &mItemCostProvider;

        std::vector<MarketOrder> mData;

        size_t mTotalOrders = 0;
        quint64 mVolumeRemaining = 0;
        quint64 mVolumeEntered = 0;
        double mTotalISK = 0.;
        double mTotalSize = 0.;

        Character::IdType mCharacterId = Character::invalidId;
    };
}
