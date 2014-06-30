#pragma once

#include <vector>

#include <QDateTime>

#include "Character.h"
#include "Entity.h"
#include "Key.h"

namespace Evernus
{
    class CachedCharacterList
        : public Entity<Key::IdType>
    {
    public:
        typedef std::vector<Character::IdType> CharacterList;

        using Entity::Entity;
        virtual ~CachedCharacterList() = default;

        CharacterList getCharacterList() const;
        void setCharacterList(const CharacterList &list);
        void setCharacterList(CharacterList &&list);

        QDateTime getCacheUntil() const;
        void setCacheUntil(const QDateTime &dt);

    private:
        CharacterList mCharacterList;
        QDateTime mCacheUntil;
    };
}
