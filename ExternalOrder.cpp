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

    double ExternalOrder::getValue() const noexcept
    {
        return mValue;
    }

    void ExternalOrder::setValue(double value) noexcept
    {
        mValue = value;
    }
}
