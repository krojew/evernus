#pragma once

#include <QDateTime>

#include "Character.h"

namespace Evernus
{
    class CachedEntity
        : public Entity<Character::IdType>
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
