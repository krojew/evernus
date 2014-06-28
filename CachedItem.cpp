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

    CachedAssetList::IdType CachedItem::getListId() const noexcept
    {
        return mListId;
    }

    void CachedItem::setListId(CachedAssetList::IdType id) noexcept
    {
        mListId = id;
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

    ItemData CachedItem::getItemData() const &
    {
        return mData;
    }

    ItemData &&CachedItem::getItemData() && noexcept
    {
        return std::move(mData);
    }

    void CachedItem::setItemData(const ItemData &data)
    {
        mData = data;
    }

    void CachedItem::setItemData(ItemData &&data)
    {
        mData = std::move(data);
    }
}
