#include "CachedAssetList.h"

namespace Evernus
{
    Key::IdType CachedAssetList::getKeyId() const noexcept
    {
        return mKeyId;
    }

    void CachedAssetList::setKeyId(Key::IdType id) noexcept
    {
        mKeyId = id;
    }

    Character::IdType CachedAssetList::getCharacterId() const noexcept
    {
        return mCharacterId;
    }

    void CachedAssetList::setCharacterId(Character::IdType id) noexcept
    {
        mCharacterId = id;
    }

    QDateTime CachedAssetList::getCacheUntil() const
    {
        return mCacheUntil;
    }

    void CachedAssetList::setCacheUntil(const QDateTime &dt)
    {
        mCacheUntil = dt;
    }
}
