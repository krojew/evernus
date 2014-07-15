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

#include <QAbstractItemModel>

namespace Evernus
{
    class MarketOrderModel
        : public QAbstractItemModel
    {
    public:
        using QAbstractItemModel::QAbstractItemModel;
        virtual ~MarketOrderModel() = default;

        virtual uint getOrderCount() const = 0;
        virtual quint64 getVolumeRemaining() const = 0;
        virtual quint64 getVolumeEntered() const = 0;
        virtual double getTotalISK() const = 0;
        virtual double getTotalSize() const = 0;
    };
}
