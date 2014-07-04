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

#include <QDateTime>

#include "ItemData.h"
#include "Entity.h"

namespace Evernus
{
    class ItemPrice
        : public Entity<uint>
    {
    public:
        typedef ItemData::TypeIdType TypeIdType;
        typedef ItemData::LocationIdType::value_type LocationIdType;

        enum class Type
        {
            Buy,
            Sell
        };

        using Entity::Entity;
        virtual ~ItemPrice() = default;

        Type getType() const noexcept;
        void setType(Type type) noexcept;

        TypeIdType getTypeId() const noexcept;
        void setTypeId(TypeIdType id) noexcept;

        LocationIdType getLocationId() const noexcept;
        void setLocationId(LocationIdType id) noexcept;

        QDateTime getUpdateTime() const;
        void setUpdateTime(const QDateTime &dt);

        double getValue() const noexcept;
        void setValue(double value) noexcept;

    private:
        Type mType = Type::Buy;
        TypeIdType mTypeId = TypeIdType{};
        LocationIdType mLocationId = LocationIdType{};
        QDateTime mUpdateTime;
        double mValue = 0.;
    };
}
