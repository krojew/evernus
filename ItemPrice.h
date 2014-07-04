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
