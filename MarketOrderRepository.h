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
#include <unordered_set>

#include "MarketOrder.h"
#include "Repository.h"

namespace Evernus
{
    class MarketOrderRepository
        : public Repository<MarketOrder>
    {
    public:
        struct SingleAggrData
        {
            uint mCount = 0;
            double mPriceSum = 0.;
        };

        struct AggrData
        {
            SingleAggrData mBuyData;
            SingleAggrData mSellData;
        };

        struct OrderState
        {
            MarketOrder::State mState = MarketOrder::State::Archived;
            uint mVolumeRemaining = 0;
            QDateTime mFirstSeen;
        };

        typedef std::unordered_map<MarketOrder::IdType, OrderState> OrderStateMap;
        typedef std::unordered_set<MarketOrder::IdType> OrderIdList;
        typedef std::vector<MarketOrder> OrderList;

        using Repository::Repository;
        virtual ~MarketOrderRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual MarketOrder populate(const QSqlRecord &record) const override;

        void create(const Repository<Character> &characterRepo) const;

        AggrData getAggregatedData(Character::IdType characterId) const;
        OrderStateMap getOrderStatesAndVolumes(Character::IdType characterId) const;

        void archive(const OrderIdList &list) const;

        OrderList fetchForCharacter(Character::IdType characterId, MarketOrder::Type type) const;
        OrderList fetchArchivedForCharacter(Character::IdType characterId) const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const MarketOrder &entity, QSqlQuery &query) const override;
        virtual void bindPositionalValues(const MarketOrder &entity, QSqlQuery &query) const override;

        virtual size_t getMaxRowsPerInsert() const override;
    };
}
