#pragma once

#include <QDateTime>

#include "Character.h"
#include "Entity.h"

namespace Evernus
{
    class CachedAssetList
        : public Entity<uint>
    {
    public:
        using Entity::Entity;
        virtual ~CachedAssetList() = default;

        Character::IdType getCharacterId() const noexcept;
        void setCharacterId(Character::IdType id) noexcept;

        QDateTime getCacheUntil() const;
        void setCacheUntil(const QDateTime &dt);

    private:
        Character::IdType mCharacterId = Character::invalidId;
        QDateTime mCacheUntil;
    };
}
