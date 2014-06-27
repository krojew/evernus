#pragma once

#include "CachedEntity.h"
#include "Character.h"

namespace Evernus
{
    class CachedAssetList
        : public CachedEntity
    {
    public:
        using CachedEntity::CachedEntity;
        virtual ~CachedAssetList() = default;

        Character::IdType getCharacterId() const noexcept;
        void setCharacterId(Character::IdType id) noexcept;

    private:
        Character::IdType mCharacterId = Character::invalidId;
    };
}
