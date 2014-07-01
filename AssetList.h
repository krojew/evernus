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

        AssetList() = default;
        AssetList(const AssetList &other);
        AssetList(AssetList &&) = default;

        template<class T>
        explicit AssetList(T &&items)
            : Entity{}
            , mItems{std::forward<T>(items)}
        {
        }

        virtual ~AssetList();

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
            item->setListId(getId());
            mItems.emplace_back(std::forward<T>(item));
        }

        AssetList &operator =(const AssetList &other);
        AssetList &operator =(AssetList &&) = default;

    private:
        Character::IdType mCharacterId = Character::invalidId;
        ItemList mItems;
    };
}
