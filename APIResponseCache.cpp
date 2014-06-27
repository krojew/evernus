#include "DatabaseUtils.h"

#include "APIResponseCache.h"

namespace Evernus
{
    APIResponseCache::APIResponseCache()
        : mCacheDb{QSqlDatabase::addDatabase("QSQLITE", "cache")}
    {
        createDb();
        createDbSchema();
        clearOldData();
        refreshCaches();
    }

    bool APIResponseCache::hasChracterListData(Key::IdType key) const
    {
        const auto it = mCharacterListCache.find(key);
        if (it == std::end(mCharacterListCache))
            return false;

        if (QDateTime::currentDateTimeUtc() > it->second.mCacheUntil)
        {
            mCharacterListCache.erase(it);
            mCharacterListRepository->remove(key);
            return false;
        }

        return true;
    }

    APIResponseCache::CharacterList APIResponseCache::getCharacterListData(Key::IdType key) const
    {
        const auto it = mCharacterListCache.find(key);
        Q_ASSERT(it != std::end(mCharacterListCache));

        return it->second.mData;
    }

    void APIResponseCache::setChracterListData(Key::IdType key, const CharacterList &data, const QDateTime &cacheUntil)
    {
        CacheEntry<CharacterList> entry;
        entry.mCacheUntil = cacheUntil;
        entry.mData = data;

        mCharacterListCache.emplace(key, std::move(entry));

        CachedCharacterList cachedEntry;
        cachedEntry.setId(key);
        cachedEntry.setCacheUntil(cacheUntil);
        cachedEntry.setCharacterList(data);

        mCharacterListRepository->store(cachedEntry);
    }

    bool APIResponseCache::hasCharacterData(Key::IdType key, Character::IdType characterId) const
    {
        const auto it = mCharacterCache.find(std::make_pair(key, characterId));
        if (it == std::end(mCharacterCache))
            return false;

        if (QDateTime::currentDateTimeUtc() > it->second.mCacheUntil)
        {
            mCharacterCache.erase(it);

            auto query = mCharacterRepository->prepare(QString{"DELETE FROM %1 WHERE %2 = :key_id AND character_id = :character_id"}
                .arg(mCharacterRepository->getTableName())
                .arg(mCharacterRepository->getIdColumn()));
            query.bindValue(":key_id", key);
            query.bindValue(":character_id", characterId);
            query.exec();

            return false;
        }

        return true;
    }

    Character APIResponseCache::getCharacterData(Key::IdType key, Character::IdType characterId) const
    {
        const auto it = mCharacterCache.find(std::make_pair(key, characterId));
        Q_ASSERT(it != std::end(mCharacterCache));

        return it->second.mData;
    }

    void APIResponseCache::setCharacterData(Key::IdType key, Character::IdType characterId, const Character &data, const QDateTime &cacheUntil)
    {
        CacheEntry<Character> entry;
        entry.mCacheUntil = cacheUntil;
        entry.mData = data;

        mCharacterCache.emplace(std::make_pair(key, characterId), std::move(entry));

        CachedCharacter cachedEntry;
        cachedEntry.setId(key);
        cachedEntry.setCacheUntil(cacheUntil);
        cachedEntry.setCharacterId(data.getId());
        cachedEntry.setCharacterData(data.getCharacterData());

        mCharacterRepository->store(cachedEntry);
    }

    QDateTime APIResponseCache::getCharacterDataLocalCacheTime(Key::IdType key, Character::IdType characterId) const
    {
        const auto it = mCharacterCache.find(std::make_pair(key, characterId));
        if (it == std::end(mCharacterCache))
            return QDateTime::currentDateTime();

        return it->second.mCacheUntil.toLocalTime();
    }

    void APIResponseCache::createDb()
    {
        DatabaseUtils::createDb(mCacheDb, "cache.db");

        mCharacterListRepository.reset(new CachedCharacterListRepository{mCacheDb});
        mCharacterRepository.reset(new CachedCharacterRepository{mCacheDb});
    }

    void APIResponseCache::createDbSchema()
    {
        mCharacterListRepository->create();
        mCharacterRepository->create();
    }

    void APIResponseCache::clearOldData()
    {
        mCharacterListRepository->clearOldData();
        mCharacterRepository->clearOldData();
    }

    void APIResponseCache::refreshCaches()
    {
        const auto characterLists = mCharacterListRepository->fetchAll();
        for (const auto &list : characterLists)
        {
            CacheEntry<CharacterList> entry;
            entry.mCacheUntil = list.getCacheUntil();
            entry.mData = list.getCharacterList();

            mCharacterListCache.emplace(list.getId(), std::move(entry));
        }

        const auto characters = mCharacterRepository->fetchAll();
        for (const auto &character : characters)
        {
            CacheEntry<Character> entry;
            entry.mCacheUntil = character.getCacheUntil();
            entry.mData.setId(character.getCharacterId());
            entry.mData.setKeyId(character.getId());
            entry.mData.setCharacterData(std::move(character).getCharacterData());

            mCharacterCache.emplace(std::make_pair(character.getId(), character.getCharacterId()), std::move(entry));
        }
    }
}
