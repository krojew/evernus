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

    bool APIResponseCache::hasAssetData(Key::IdType key, Character::IdType characterId) const
    {
        const auto it = mAssetCache.find(std::make_pair(key, characterId));
        if (it == std::end(mAssetCache))
            return false;

        if (QDateTime::currentDateTimeUtc() > it->second.mCacheUntil)
        {
            mAssetCache.erase(it);

            auto query = mAssetListRepository->prepare(QString{"DELETE FROM %1 WHERE key_id = :key_id AND character_id = :character_id"}
                .arg(mAssetListRepository->getTableName()));
            query.bindValue(":key_id", key);
            query.bindValue(":character_id", characterId);
            query.exec();

            return false;
        }

        return true;
    }

    AssetList APIResponseCache::getAssetData(Key::IdType key, Character::IdType characterId) const
    {
        const auto it = mAssetCache.find(std::make_pair(key, characterId));
        Q_ASSERT(it != std::end(mAssetCache));

        return it->second.mData;
    }

    void APIResponseCache::setAssetData(Key::IdType key, Character::IdType characterId, const AssetList &data, const QDateTime &cacheUntil)
    {
        CacheEntry<AssetList> entry;
        entry.mCacheUntil = cacheUntil;
        entry.mData = data;

        mAssetCache.emplace(std::make_pair(key, characterId), std::move(entry));

        auto query = mAssetListRepository->prepare(QString{"DELETE FROM %1 WHERE key_id = :key_id AND character_id = :character_id"}
            .arg(mAssetListRepository->getTableName()));
        query.bindValue(":key_id", key);
        query.bindValue(":character_id", characterId);
        query.exec();

        CachedAssetList list;
        list.setKeyId(key);
        list.setCacheUntil(cacheUntil);
        list.setCharacterId(characterId);

        mAssetListRepository->store(list);

        QVariantList boundValues[CachedItemRepository::columnCount - 1];

        for (const auto &item : data)
            saveItemTree(*item, nullptr, boundValues);

        const auto maxRowsPerInsert = 100u;
        const auto batches = data.size() / maxRowsPerInsert;

        for (auto batch = 0u; batch < batches; ++batch)
        {
            query = prepareBatchItemInsertQuery(maxRowsPerInsert);

            for (auto row = batch * maxRowsPerInsert; row < (batch + 1) * maxRowsPerInsert; ++row)
            {
                query.addBindValue(list.getId());
                for (const auto &column : boundValues)
                    query.addBindValue(column[row]);
            }

            DatabaseUtils::execQuery(query);
        }

        query = prepareBatchItemInsertQuery(data.size() % maxRowsPerInsert);

        for (auto row = batches * maxRowsPerInsert; row < data.size(); ++row)
        {
            query.addBindValue(list.getId());
            for (const auto &column : boundValues)
                query.addBindValue(column[row]);
        }

        DatabaseUtils::execQuery(query);
    }

    QDateTime APIResponseCache::getAssetsDataLocalCacheTime(Key::IdType key, Character::IdType characterId) const
    {
        const auto it = mAssetCache.find(std::make_pair(key, characterId));
        if (it == std::end(mAssetCache))
            return QDateTime::currentDateTime();

        return it->second.mCacheUntil.toLocalTime();
    }

    bool APIResponseCache::hasConquerableStationListData() const
    {
        if (QDateTime::currentDateTimeUtc() >  mConquerableStationCache.mCacheUntil)
        {
            mConquerableStationCache.mData.clear();
            return false;
        }

        return !mConquerableStationCache.mData.empty();
    }

    ConquerableStationList APIResponseCache::getConquerableStationListData() const
    {
        return mConquerableStationCache.mData;
    }

    void APIResponseCache::setConquerableStationListData(const ConquerableStationList &data, const QDateTime &cacheUntil)
    {
        mConquerableStationCache.mCacheUntil = cacheUntil;
        mConquerableStationCache.mData = data;

        mConquerableStationListRepository->exec(QString{"DELETE FROM %1"}.arg(mConquerableStationListRepository->getTableName()));

        CachedConquerableStationList list;
        list.setCacheUntil(cacheUntil);

        mConquerableStationListRepository->store(list);

        const auto maxRowsPerInsert = 100u;
        const auto batches = data.size() / maxRowsPerInsert;

        for (auto batch = 0u; batch < batches; ++batch)
        {
            auto query = prepareBatchConquerableStationInsertQuery(maxRowsPerInsert);

            const auto end = std::next(std::begin(data), (batch + 1) * maxRowsPerInsert);
            for (auto it = std::next(std::begin(data), batch * maxRowsPerInsert); it != end; ++it)
            {
                query.addBindValue(it->mId);
                query.addBindValue(list.getId());
                query.addBindValue(it->mName);
            }

            DatabaseUtils::execQuery(query);
        }

        auto query = prepareBatchConquerableStationInsertQuery(data.size() % maxRowsPerInsert);

        for (auto it = std::next(std::begin(data), batches * maxRowsPerInsert); it != std::end(data); ++it)
        {
            query.addBindValue(it->mId);
            query.addBindValue(list.getId());
            query.addBindValue(it->mName);
        }

        DatabaseUtils::execQuery(query);
    }

    void APIResponseCache::createDb()
    {
        DatabaseUtils::createDb(mCacheDb, "cache.db");

        mCharacterListRepository.reset(new CachedCharacterListRepository{mCacheDb});
        mCharacterRepository.reset(new CachedCharacterRepository{mCacheDb});
        mAssetListRepository.reset(new CachedAssetListRepository{mCacheDb});
        mItemRepository.reset(new CachedItemRepository{mCacheDb});
        mConquerableStationListRepository.reset(new CachedConquerableStationListRepository{mCacheDb});
        mConquerableStationRepository.reset(new CachedConquerableStationRepository{mCacheDb});
    }

    void APIResponseCache::createDbSchema()
    {
        mCharacterListRepository->create();
        mCharacterRepository->create();
        mAssetListRepository->create();
        mItemRepository->create(*mAssetListRepository);
        mConquerableStationListRepository->create();
        mConquerableStationRepository->create(*mConquerableStationListRepository);
    }

    void APIResponseCache::clearOldData()
    {
        mCharacterListRepository->clearOldData();
        mCharacterRepository->clearOldData();
        mAssetListRepository->clearOldData();
        mConquerableStationListRepository->clearOldData();
    }

    void APIResponseCache::refreshCaches()
    {
        refreshCharacterLists();
        refreshCharacters();
        refreshAssets();
        refreshConquerableStations();
    }

    void APIResponseCache::refreshCharacterLists()
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

    void APIResponseCache::refreshCharacters()
    {
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

    void APIResponseCache::refreshAssets()
    {
        const auto assetLists = mAssetListRepository->fetchAll();
        const auto items = mItemRepository->fetchAll();

        std::unordered_map<CachedAssetList::IdType, const CachedAssetList *> assetMap;
        for (const auto &list : assetLists)
            assetMap[list.getId()] = &list;

        std::unordered_map<Item::IdType, std::unique_ptr<Item>> itemMap;
        std::unordered_map<Item::IdType, Item *> persistentItemMap;
        for (const auto &item : items)
        {
            std::unique_ptr<Item> realItem{new Item{item.getId()}};
            realItem->setItemData(std::move(item).getItemData());

            persistentItemMap.emplace(realItem->getId(), realItem.get());
            itemMap.emplace(realItem->getId(), std::move(realItem));
        }

        for (const auto &item : items)
        {
            const auto parentId = item.getParentId();
            if (parentId)
            {
                const auto it = persistentItemMap.find(*parentId);
                if (it != std::end(persistentItemMap))
                {
                    const auto child = itemMap.find(item.getId());
                    it->second->addItem(std::move(child->second));
                }
            }
        }

        for (const auto &item : items)
        {
            const auto assetList = assetMap[item.getListId()];
            if (assetList == nullptr)
                continue;

            if (!itemMap[item.getId()])
                continue;

            auto &entry = mAssetCache[std::make_pair(assetList->getKeyId(), assetList->getCharacterId())];
            if (entry.mCacheUntil.isNull())
                entry.mCacheUntil = assetList->getCacheUntil();

            entry.mData.emplace_back(std::move(itemMap[item.getId()]));
        }
    }

    void APIResponseCache::refreshConquerableStations()
    {
        auto query = mConquerableStationListRepository->exec(QString{
            "SELECT * FROM %1 ORDER BY cache_until DESC LIMIT 1"}.arg(mConquerableStationListRepository->getTableName()));
        if (!query.next())
            return;

        const auto stationList = mConquerableStationListRepository->populate(query.record());

        mConquerableStationCache.mCacheUntil = stationList.getCacheUntil();

        query = mConquerableStationRepository->prepare(QString{
            "SELECT * FROM %1 WHERE list_id = :id"}.arg(mConquerableStationRepository->getTableName()));
        query.bindValue(":id", stationList.getId());
        if (!query.exec())
            return;

        while (query.next())
        {
            const auto station = mConquerableStationRepository->populate(query.record());
            mConquerableStationCache.mData.emplace_back(ConquerableStation{station.getId(), station.getName()});
        }
    }

    void APIResponseCache::saveItemTree(const Item &item, const Item *parent, QVariantList boundValues[CachedItemRepository::columnCount - 1]) const
    {
        boundValues[0] << item.getId();
        boundValues[1] << ((parent == nullptr) ? (QVariant{QVariant::ULongLong}) : (parent->getId()));
        boundValues[2] << item.getTypeId();
        boundValues[3] << ((item.getLocationId()) ? (*item.getLocationId()) : (QVariant{QVariant::ULongLong}));
        boundValues[4] << item.getQuantity();

        for (const auto &child : item)
            saveItemTree(*child, &item, boundValues);
    }

    QSqlQuery APIResponseCache::prepareBatchConquerableStationInsertQuery(size_t numValues) const
    {
        QStringList values;
        for (auto i = 0u; i < numValues; ++i)
            values << "(?, ?, ?)";

        return mConquerableStationRepository->prepare(QString{"INSERT INTO %1 (%3, list_id, name) VALUES %2"}
            .arg(mConquerableStationRepository->getTableName())
            .arg(values.join(", "))
            .arg(mConquerableStationRepository->getIdColumn()));
    }

    QSqlQuery APIResponseCache::prepareBatchItemInsertQuery(size_t numValues) const
    {
        QStringList columns;
        for (auto j = 0; j < CachedItemRepository::columnCount; ++j)
            columns << "?";

        const auto columnBindings = QString{"(%1)"}.arg(columns.join(", "));

        QStringList values;
        for (auto i = 0u; i < numValues; ++i)
            values << columnBindings;

        return mItemRepository->prepare(QString{"INSERT INTO %1 (asset_list_id, %3, parent_id, type_id, location_id, quantity) VALUES %2"}
            .arg(mItemRepository->getTableName())
            .arg(values.join(", "))
            .arg(mItemRepository->getIdColumn()));
    }
}
