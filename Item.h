#pragma once

#include <vector>
#include <memory>

#include "ItemData.h"

namespace Evernus
{
    class Item
    {
    public:


        typedef std::vector<std::unique_ptr<Item>> ItemList;
        typedef ItemList::iterator ItemIterator;
        typedef ItemList::const_iterator ConstItemIterator;

        Item() = default;
        explicit Item(ItemData::IdType id);
        virtual ~Item() = default;

        ItemData::IdType getId() const noexcept;
        void setId(ItemData::IdType id) noexcept;

        ItemData::TypeIdType getTypeId() const;
        void setTypeId(const ItemData::TypeIdType &id);

        ItemData::LocationIdType getLocationId() const;
        void setLocationId(const ItemData::LocationIdType &id);

        uint getQuantity() const noexcept;
        void setQuantity(uint value) noexcept;

        ItemIterator begin() noexcept;
        ConstItemIterator begin() const noexcept;
        ItemIterator end() noexcept;
        ConstItemIterator end() const noexcept;

        void addItem(std::unique_ptr<Item> &&item);

    private:
        ItemData::IdType mId = 0;
        ItemData mData;
        ItemList mContents;
    };
}
