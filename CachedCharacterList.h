#pragma once

#include <vector>

#include "CachedEntity.h"
#include "Character.h"

namespace Evernus
{
    class CachedCharacterList
        : public CachedEntity
    {
    public:
        typedef std::vector<Character::IdType> CharacterList;

        using CachedEntity::CachedEntity;
        virtual ~CachedCharacterList() = default;

        CharacterList getCharacterList() const;
        void setCharacterList(const CharacterList &list);
        void setCharacterList(CharacterList &&list);

    private:
        CharacterList mCharacterList;
    };
}
