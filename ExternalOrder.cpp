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
#include "ExternalOrder.h"

namespace Evernus
{
    ExternalOrder::Type ExternalOrder::getType() const noexcept
    {
        return mType;
    }

    void ExternalOrder::setType(Type type) noexcept
    {
        mType = type;
    }

    ExternalOrder::TypeIdType ExternalOrder::getTypeId() const noexcept
    {
        return mTypeId;
    }

    void ExternalOrder::setTypeId(TypeIdType id) noexcept
    {
        mTypeId = id;
    }

    ExternalOrder::LocationIdType ExternalOrder::getLocationId() const noexcept
    {
        return mLocationId;
    }

    void ExternalOrder::setLocationId(LocationIdType id) noexcept
    {
        mLocationId = id;
    }

    uint ExternalOrder::getSolarSystemId() const noexcept
    {
        return mSolarSystemId;
    }

    void ExternalOrder::setSolarSystemId(uint id) noexcept
    {
        mSolarSystemId = id;
    }

    uint ExternalOrder::getRegionId() const noexcept
    {
        return mRegionId;
    }

    void ExternalOrder::setRegionId(uint id) noexcept
    {
        mRegionId = id;
    }

    short ExternalOrder::getRange() const noexcept
    {
        return mRange;
    }

    void ExternalOrder::setRange(short value) noexcept
    {
        mRange = value;
    }

    QDateTime ExternalOrder::getUpdateTime() const
    {
        return mUpdateTime;
    }

    void ExternalOrder::setUpdateTime(const QDateTime &dt)
    {
        mUpdateTime = dt;
    }

    double ExternalOrder::getPrice() const noexcept
    {
        return mPrice;
    }

    void ExternalOrder::setPrice(double value) noexcept
    {
        mPrice = value;
    }

    ExternalOrder ExternalOrder::parseLogLine(const QStringList &values)
    {
        const auto priceColumn = 0;
        const auto typeColumn = 2;
        const auto rangeColumn = 3;
        const auto idColumn = 4;
        const auto bidColumn = 7;
        const auto stationColumn = 10;
        const auto regionColumn = 11;
        const auto systemColumn = 12;

        ExternalOrder order{values[idColumn].toULongLong()};
        order.setLocationId(values[stationColumn].toULongLong());
        order.setSolarSystemId(values[systemColumn].toUInt());
        order.setRegionId(values[regionColumn].toUInt());
        order.setRange(values[rangeColumn].toShort());
        order.setType((values[bidColumn] == "True") ? (ExternalOrder::Type::Buy) : (ExternalOrder::Type::Sell));
        order.setTypeId(values[typeColumn].toULongLong());
        order.setPrice(values[priceColumn].toDouble());

        return order;
    }
}
