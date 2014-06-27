#include "CachedItem.h"

namespace Evernus
{
    CachedItem::ParentIdType CachedItem::getParentId() const noexcept
    {
        return mParentId;
    }

    void CachedItem::setParentId(const ParentIdType &id) noexcept
    {
        mParentId = id;
    }

    ItemData::TypeIdType CachedItem::getTypeId() const
    {
        return mData.mTypeId;
    }

    void CachedItem::setTypeId(const ItemData::TypeIdType &id)
    {
        mData.mTypeId = id;
    }

    ItemData::LocationIdType CachedItem::getLocationId() const
    {
        return mData.mLocationId;
    }

    void CachedItem::setLocationId(const ItemData::LocationIdType &id)
    {
        mData.mLocationId = id;
    }

    uint CachedItem::getQuantity() const noexcept
    {
        return mData.mQuantity;
    }

    void CachedItem::setQuantity(uint value) noexcept
    {
        mData.mQuantity = value;
    }
}
