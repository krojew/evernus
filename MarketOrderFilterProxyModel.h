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

#include "LeafFilterProxyModel.h"

namespace Evernus
{
    class MarketOrderFilterProxyModel
        : public LeafFilterProxyModel
    {
        Q_OBJECT

    public:
        enum StatusFilter
        {
            Changed             = 0x01,
            Active              = 0x02,
            Fulfilled           = 0x04,
            Cancelled           = 0x08,
            Pending             = 0x10,
            CharacterDeleted    = 0x20,
            Expired             = 0x40,

            All                 = Changed | Active | Fulfilled | Cancelled | Pending | CharacterDeleted | Expired
        };
        Q_DECLARE_FLAGS(StatusFilters, StatusFilter)

        static const StatusFilters defaultFilter;

        explicit MarketOrderFilterProxyModel(QObject *parent = nullptr);
        virtual ~MarketOrderFilterProxyModel() = default;

        virtual void setSourceModel(QAbstractItemModel *sourceModel) override;

    public slots:
        void setStatusFilter(const StatusFilters &filter);

    protected:
        virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
        StatusFilters mStatusFilter = defaultFilter;
    };
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Evernus::MarketOrderFilterProxyModel::StatusFilters)
