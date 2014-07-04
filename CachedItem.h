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

#include "CachedAssetList.h"
#include "ItemData.h"
#include "Entity.h"

namespace Evernus
{
    class CachedItem
        : public Entity<ItemData::IdType>
    {
    public:
        typedef boost::optional<ItemData::IdType> ParentIdType;

        using Entity::Entity;
        virtual ~CachedItem() = default;

        ParentIdType getParentId() const noexcept;
        void setParentId(const ParentIdType &id) noexcept;

        CachedAssetList::IdType getListId() const noexcept;
        void setListId(CachedAssetList::IdType id) noexcept;

        ItemData::TypeIdType getTypeId() const;
        void setTypeId(const ItemData::TypeIdType &id);

        ItemData::LocationIdType getLocationId() const;
        void setLocationId(const ItemData::LocationIdType &id);

        uint getQuantity() const noexcept;
        void setQuantity(uint value) noexcept;

        ItemData getItemData() const &;
        ItemData &&getItemData() && noexcept;
        void setItemData(const ItemData &data);
        void setItemData(ItemData &&data);

    private:
        ParentIdType mParentId;
        CachedAssetList::IdType mListId = CachedAssetList::invalidId;
        ItemData mData;
    };
}
