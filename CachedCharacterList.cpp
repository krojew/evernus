#include "CachedCharacterList.h"

namespace Evernus
{
    CachedCharacterList::CharacterList CachedCharacterList::getCharacterList() const
    {
        return mCharacterList;
    }

    void CachedCharacterList::setCharacterList(const CharacterList &list)
    {
        mCharacterList = list;
    }

    void CachedCharacterList::setCharacterList(CharacterList &&list)
    {
        mCharacterList = std::move(list);
    }

    QDateTime CachedCharacterList::getCacheUntil() const
    {
        return mCacheUntil;
    }

    void CachedCharacterList::setCacheUntil(const QDateTime &dt)
    {
        mCacheUntil = dt;
    }
}
