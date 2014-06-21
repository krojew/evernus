#pragma once

#include <QDateTime>

#include "Key.h"

namespace Evernus
{
    class CachedEntity
        : public Entity<Key::IdType>
    {
    public:
        using Entity::Entity;
        virtual ~CachedEntity() = default;

        QDateTime getCacheUntil() const;
        void setCacheUntil(const QDateTime &dt);

    private:
        QDateTime mCacheUntil;
    };
}
