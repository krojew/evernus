#pragma once

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

        ItemData::TypeIdType getTypeId() const;
        void setTypeId(const ItemData::TypeIdType &id);

        ItemData::LocationIdType getLocationId() const;
        void setLocationId(const ItemData::LocationIdType &id);

        uint getQuantity() const noexcept;
        void setQuantity(uint value) noexcept;

    private:
        ParentIdType mParentId;
        ItemData mData;
    };
}
