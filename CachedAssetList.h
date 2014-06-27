#pragma once

#include <QDateTime>

#include "Character.h"
#include "Entity.h"
#include "Key.h"

namespace Evernus
{
    class CachedAssetList
        : public Entity<uint>
    {
    public:
        using Entity::Entity;
        virtual ~CachedAssetList() = default;

        Key::IdType getKeyId() const noexcept;
        void setKeyId(Key::IdType id) noexcept;

        Character::IdType getCharacterId() const noexcept;
        void setCharacterId(Character::IdType id) noexcept;

        QDateTime getCacheUntil() const;
        void setCacheUntil(const QDateTime &dt);

    private:
        Key::IdType mKeyId = Key::invalidId;
        Character::IdType mCharacterId = Character::invalidId;
        QDateTime mCacheUntil;
    };
}
