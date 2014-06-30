#pragma once

#include <QDateTime>

#include "Entity.h"

namespace Evernus
{
    class CachedConquerableStationList
        : public Entity<uint>
    {
    public:
        using Entity::Entity;
        virtual ~CachedConquerableStationList() = default;

        QDateTime getCacheUntil() const;
        void setCacheUntil(const QDateTime &dt);

    private:
        QDateTime mCacheUntil;
    };
}
