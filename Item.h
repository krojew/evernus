#pragma once

#include <vector>
#include <memory>

#include "AssetList.h"
#include "ItemData.h"
#include "Entity.h"

namespace Evernus
{
    class Item
        : public Entity<ItemData::IdType>
    {
    public:
        typedef std::vector<std::unique_ptr<Item>> ItemList;
        typedef ItemList::iterator ItemIterator;
        typedef ItemList::const_iterator ConstItemIterator;

        typedef boost::optional<ItemData::IdType> ParentIdType;

        using Entity::Entity;

        Item() = default;
        Item(const Item &other);
        Item(Item &&) = default;
        virtual ~Item() = default;

        ParentIdType getParentId() const noexcept;
        void setParentId(const ParentIdType &id) noexcept;

        AssetList::IdType getListId() const noexcept;
        void setListId(AssetList::IdType id) noexcept;

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

        ItemIterator begin() noexcept;
        ConstItemIterator begin() const noexcept;
        ItemIterator end() noexcept;
        ConstItemIterator end() const noexcept;

        size_t getChildCount() const noexcept;

        void addItem(std::unique_ptr<Item> &&item);

        Item &operator =(const Item &other);
        Item &operator =(Item &&) = default;

    private:
        ParentIdType mParentId;
        AssetList::IdType mListId = AssetList::invalidId;
        ItemData mData;
        ItemList mContents;
    };
}
