#include "CachedEntity.h"

namespace Evernus
{
    QDateTime CachedEntity::getCacheUntil() const
    {
        return mCacheUntil;
    }

    void CachedEntity::setCacheUntil(const QDateTime &dt)
    {
        mCacheUntil = dt;
    }
}
