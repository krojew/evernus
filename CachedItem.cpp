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
