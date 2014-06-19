#pragma once

#include <unordered_map>
#include <vector>

#include <QDateTime>

#include "Character.h"
#include "Key.h"

namespace Evernus
{
    class APIResponseCache
    {
    public:
        typedef std::vector<Character::IdType> CharacterList;

        APIResponseCache() = default;
        virtual ~APIResponseCache() = default;

        bool hasChracterListData(Key::IdType key) const;
        CharacterList getCharacterListData(Key::IdType key) const;

    private:
        template<class T>
        struct CacheEntry
        {
            QDateTime mCachedUntil;
            T mData;
        };

        mutable std::unordered_map<Key::IdType, CacheEntry<CharacterList>> mCharacterListCache;
    };
}
