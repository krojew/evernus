#include "Item.h"

#include "AssetList.h"

namespace Evernus
{
    AssetList::AssetList(const AssetList &other)
        : Entity{other}
        , mCharacterId{other.mCharacterId}
    {
        for (const auto &item : other)
            addItem(std::make_unique<Item>(*item));
    }

    AssetList::~AssetList()
    {
    }

    Character::IdType AssetList::getCharacterId() const noexcept
    {
        return mCharacterId;
    }

    void AssetList::setCharacterId(Character::IdType id) noexcept
    {
        mCharacterId = id;
    }

    AssetList::Iterator AssetList::begin() noexcept
    {
        return mItems.begin();
    }

    AssetList::ConstIterator AssetList::begin() const noexcept
    {
        return mItems.begin();
    }

    AssetList::Iterator AssetList::end() noexcept
    {
        return mItems.end();
    }

    AssetList::ConstIterator AssetList::end() const noexcept
    {
        return mItems.end();
    }

    size_t AssetList::size() const noexcept
    {
        return mItems.size();
    }

    AssetList &AssetList::operator =(const AssetList &other)
    {
        using std::swap;

        AssetList copy{other};
        swap(*this, copy);

        return *this;
    }
}
