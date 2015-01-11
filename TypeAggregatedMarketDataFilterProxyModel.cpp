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
#include "TypeAggregatedMarketDataFilterProxyModel.h"

namespace Evernus
{
    TypeAggregatedMarketDataFilterProxyModel::TypeAggregatedMarketDataFilterProxyModel(int volumeColumn, int marginColumn, QObject *parent)
        : QSortFilterProxyModel{parent}
        , mVolumeColumn{volumeColumn}
        , mMarginColumn{marginColumn}
    {
    }

    void TypeAggregatedMarketDataFilterProxyModel
    ::setFilter(VolumeValueType minVolume, VolumeValueType maxVolume, MarginValueType minMargin, MarginValueType maxMargin)
    {
        mMinVolume = std::move(minVolume);
        mMaxVolume = std::move(maxVolume);
        mMinMargin = std::move(minMargin);
        mMaxMargin = std::move(maxMargin);

        invalidateFilter();
    }

    bool TypeAggregatedMarketDataFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent))
            return false;

        const auto source = sourceModel();
        if (source != nullptr)
        {
            if (mMinVolume || mMaxVolume)
            {
                const auto volume = source->data(source->index(sourceRow, mVolumeColumn, sourceParent), Qt::UserRole).toUInt();
                if (mMinVolume && volume < *mMinVolume)
                    return false;
                if (mMaxVolume && volume > *mMaxVolume)
                    return false;
            }

            if (mMinMargin || mMaxMargin)
            {
                const auto margin = source->data(source->index(sourceRow, mMarginColumn, sourceParent), Qt::UserRole).toDouble();
                if (mMinMargin && margin < *mMinMargin)
                    return false;
                if (mMaxMargin && margin > *mMaxMargin)
                    return false;
            }
        }

        return true;
    }
}
