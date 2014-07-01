#include "AssetList.h"

namespace Evernus
{
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
}
