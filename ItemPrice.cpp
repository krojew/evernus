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
