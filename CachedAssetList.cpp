#include "CachedAssetList.h"

namespace Evernus
{
    Character::IdType CachedAssetList::getCharacterId() const noexcept
    {
        return mCharacterId;
    }

    void CachedAssetList::setCharacterId(Character::IdType id) noexcept
    {
        mCharacterId = id;
    }
}
