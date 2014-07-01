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
