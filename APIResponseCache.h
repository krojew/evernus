#pragma once

#include <unordered_map>
#include <vector>

#include <QSqlDatabase>
#include <QDateTime>

#include "Character.h"
#include "Key.h"

namespace Evernus
{
    class APIResponseCache
    {
    public:
        typedef std::vector<Character::IdType> CharacterList;

        APIResponseCache();
        virtual ~APIResponseCache() = default;

        bool hasChracterListData(Key::IdType key) const;
        CharacterList getCharacterListData(Key::IdType key) const;
        void setChracterListData(Key::IdType key, const CharacterList &data, const QDateTime &cacheUntil);

    private:
        template<class T>
        struct CacheEntry
        {
            QDateTime mCachedUntil;
            T mData;
        };

        mutable std::unordered_map<Key::IdType, CacheEntry<CharacterList>> mCharacterListCache;

        QSqlDatabase mCacheDb;

        void createDb();
        void createDbSchema();
    };
}
