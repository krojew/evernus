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
#include "MarketOrderModel.h"
#include "MarketOrder.h"

#include "MarketOrderFilterProxyModel.h"

namespace Evernus
{
    void MarketOrderFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
    {
        Q_ASSERT(dynamic_cast<MarketOrderModel *>(sourceModel) != nullptr);
        LeafFilterProxyModel::setSourceModel(sourceModel);
    }

    void MarketOrderFilterProxyModel::setStatusFilter(const StatusFilters &filter)
    {
        mStatusFilter = filter;
        invalidateFilter();
    }

    bool MarketOrderFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        const auto parentResult = LeafFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
        if (!parentResult)
            return false;

        const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
        const auto order = static_cast<MarketOrderModel *>(sourceModel())->getOrder(index);

        if (order == nullptr)
            return true;

        if ((mStatusFilter & Changed) && (order->getDelta() != 0))
            return true;
        if ((mStatusFilter & Active) && (order->getState() == MarketOrder::State::Active))
            return true;
        if ((mStatusFilter & Fulfilled) && (order->getState() == MarketOrder::State::Fulfilled) && (order->getVolumeRemaining() == 0))
            return true;
        if ((mStatusFilter & Cancelled) && (order->getState() == MarketOrder::State::Cancelled))
            return true;
        if ((mStatusFilter & Pending) && (order->getState() == MarketOrder::State::Pending))
            return true;
        if ((mStatusFilter & CharacterDeleted) && (order->getState() == MarketOrder::State::CharacterDeleted))
            return true;
        if ((mStatusFilter & Expired) && (order->getState() == MarketOrder::State::Fulfilled) && (order->getVolumeRemaining() != 0))
            return true;

        return false;
    }
}
