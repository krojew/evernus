#pragma once

#include <memory>
#include <vector>

#include "Character.h"
#include "Entity.h"

namespace Evernus
{
    class Item;

    class AssetList
        : public Entity<uint>
    {
    public:
        typedef std::unique_ptr<Item> ItemType;
        typedef std::vector<ItemType> ItemList;
        typedef ItemList::iterator Iterator;
        typedef ItemList::const_iterator ConstIterator;

        using Entity::Entity;

        AssetList();
        AssetList(const AssetList &other);
        AssetList(AssetList &&other) = default;
        explicit AssetList(ItemList &&items);

        virtual ~AssetList();

        Character::IdType getCharacterId() const noexcept;
        void setCharacterId(Character::IdType id) noexcept;

        Iterator begin() noexcept;
        ConstIterator begin() const noexcept;
        Iterator end() noexcept;
        ConstIterator end() const noexcept;

        size_t size() const noexcept;

        void addItem(ItemType &&item);

        AssetList &operator =(AssetList other);

    private:
        Character::IdType mCharacterId = Character::invalidId;
        ItemList mItems;
    };
}
