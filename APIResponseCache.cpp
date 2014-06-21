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

    void APIResponseCache::createDb()
    {
        DatabaseUtils::createDb(mCacheDb, "cache.db");

        mCharacterListRepository.reset(new CachedCharacterListRepository{mCacheDb});
    }

    void APIResponseCache::createDbSchema()
    {
        mCharacterListRepository->create();
    }

    void APIResponseCache::clearOldData()
    {
        mCharacterListRepository->clearOldData();
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
    }
}
