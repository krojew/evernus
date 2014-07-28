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
#include "ItemPrice.h"

namespace Evernus
{
    ItemPrice::Type ItemPrice::getType() const noexcept
    {
        return mType;
    }

    void ItemPrice::setType(Type type) noexcept
    {
        mType = type;
    }

    ItemPrice::TypeIdType ItemPrice::getTypeId() const noexcept
    {
        return mTypeId;
    }

    void ItemPrice::setTypeId(TypeIdType id) noexcept
    {
        mTypeId = id;
    }

    ItemPrice::LocationIdType ItemPrice::getLocationId() const noexcept
    {
        return mLocationId;
    }

    void ItemPrice::setLocationId(LocationIdType id) noexcept
    {
        mLocationId = id;
    }

    uint ItemPrice::getSolarSystemId() const noexcept
    {
        return mSolarSystemId;
    }

    void ItemPrice::setSolarSystemId(uint id) noexcept
    {
        mSolarSystemId = id;
    }

    uint ItemPrice::getRegionId() const noexcept
    {
        return mRegionId;
    }

    void ItemPrice::setRegionId(uint id) noexcept
    {
        mRegionId = id;
    }

    short ItemPrice::getRange() const noexcept
    {
        return mRange;
    }

    void ItemPrice::setRange(short value) noexcept
    {
        mRange = value;
    }

    QDateTime ItemPrice::getUpdateTime() const
    {
        return mUpdateTime;
    }

    void ItemPrice::setUpdateTime(const QDateTime &dt)
    {
        mUpdateTime = dt;
    }

    double ItemPrice::getValue() const noexcept
    {
        return mValue;
    }

    void ItemPrice::setValue(double value) noexcept
    {
        mValue = value;
    }
}
