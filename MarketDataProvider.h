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

#include "TypeAggregatedMarketDataModel.h"

namespace Evernus
{
    class ExternalOrder;

    class MarketDataProvider
    {
    public:
        using HistoryMap = TypeAggregatedMarketDataModel::HistoryMap;
        using OrderResultType = std::vector<ExternalOrder>;

        MarketDataProvider() = default;
        MarketDataProvider(const MarketDataProvider &) = default;
        MarketDataProvider(MarketDataProvider &&) = default;
        virtual ~MarketDataProvider() = default;

        virtual const HistoryMap *getHistory(uint regionId) const = 0;
        virtual const OrderResultType *getOrders() const = 0;

        MarketDataProvider &operator =(const MarketDataProvider &) = default;
        MarketDataProvider &operator =(MarketDataProvider &&) = default;
    };
}
