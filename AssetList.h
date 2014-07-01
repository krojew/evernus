#pragma once

#include "Character.h"
#include "Entity.h"
#include "Item.h"

namespace Evernus
{
    class AssetList
        : public Entity<uint>
    {
    public:
        typedef std::shared_ptr<Item> ItemType;
        typedef std::vector<ItemType> ItemList;
        typedef ItemList::iterator Iterator;
        typedef ItemList::const_iterator ConstIterator;

        using Entity::Entity;

        AssetList() = default;
        AssetList(const AssetList &) = default;
        AssetList(AssetList &&) = default;

        template<class T>
        explicit AssetList(T &&items)
            : Entity{}
            , mItems{std::forward<T>(items)}
        {
        }

        virtual ~AssetList() = default;

        Character::IdType getCharacterId() const noexcept;
        void setCharacterId(Character::IdType id) noexcept;

        Iterator begin() noexcept;
        ConstIterator begin() const noexcept;
        Iterator end() noexcept;
        ConstIterator end() const noexcept;

        size_t size() const noexcept;

        template<class T>
        void addItem(T &&item)
        {
            mItems.emplace_back(std::forward<T>(item));
        }

        AssetList &operator =(const AssetList &) = default;
        AssetList &operator =(AssetList &&) = default;

    private:
        Character::IdType mCharacterId = Character::invalidId;
        ItemList mItems;
    };
}
