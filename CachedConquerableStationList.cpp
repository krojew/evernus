#include "CachedConquerableStationList.h"

namespace Evernus
{
    QDateTime CachedConquerableStationList::getCacheUntil() const
    {
        return mCacheUntil;
    }

    void CachedConquerableStationList::setCacheUntil(const QDateTime &dt)
    {
        mCacheUntil = dt;
    }
}
