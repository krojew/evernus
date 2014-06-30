#pragma once

#include <unordered_map>
#include <memory>
#include <vector>

#include <boost/functional/hash.hpp>

#include <QSqlDatabase>
#include <QDateTime>

#include "CachedConquerableStationListRepository.h"
#include "CachedConquerableStationRepository.h"
#include "CachedCharacterListRepository.h"
#include "CachedCharacterRepository.h"
#include "CachedAssetListRepository.h"
#include "ConquerableStationList.h"
#include "CachedItemRepository.h"
#include "AssetList.h"
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

        bool hasCharacterData(Key::IdType key, Character::IdType characterId) const;
        Character getCharacterData(Key::IdType key, Character::IdType characterId) const;
        void setCharacterData(Key::IdType key, Character::IdType characterId, const Character &data, const QDateTime &cacheUntil);

        QDateTime getCharacterDataLocalCacheTime(Key::IdType key, Character::IdType characterId) const;

        bool hasAssetData(Key::IdType key, Character::IdType characterId) const;
        AssetList getAssetData(Key::IdType key, Character::IdType characterId) const;
        void setAssetData(Key::IdType key, Character::IdType characterId, const AssetList &data, const QDateTime &cacheUntil);

        bool hasConquerableStationListData() const;
        ConquerableStationList getConquerableStationListData() const;
        void setConquerableStationListData(const ConquerableStationList &data, const QDateTime &cacheUntil);

    private:
        template<class T>
        struct CacheEntry
        {
            QDateTime mCacheUntil;
            T mData;
        };

        typedef std::pair<Key::IdType, Character::IdType> KeyCharacterPair;

        mutable std::unordered_map<Key::IdType, CacheEntry<CharacterList>> mCharacterListCache;
        mutable std::unordered_map<KeyCharacterPair, CacheEntry<Character>, boost::hash<KeyCharacterPair>> mCharacterCache;
        mutable std::unordered_map<KeyCharacterPair, CacheEntry<AssetList>, boost::hash<KeyCharacterPair>> mAssetCache;
        mutable CacheEntry<ConquerableStationList> mConquerableStationCache;

        QSqlDatabase mCacheDb;

        std::unique_ptr<CachedCharacterListRepository> mCharacterListRepository;
        std::unique_ptr<CachedCharacterRepository> mCharacterRepository;
        std::unique_ptr<CachedAssetListRepository> mAssetListRepository;
        std::unique_ptr<CachedItemRepository> mItemRepository;
        std::unique_ptr<CachedConquerableStationListRepository> mConquerableStationListRepository;
        std::unique_ptr<CachedConquerableStationRepository> mConquerableStationRepository;

        void createDb();
        void createDbSchema();
        void clearOldData();

        void refreshCaches();
        void refreshCharacterLists();
        void refreshCharacters();
        void refreshAssets();
        void refreshConquerableStations();

        void saveItemTree(const Item &item, const Item *parent, CachedAssetList::IdType listId) const;

        QSqlQuery prepareBatchConquerableStationInsertQuery(size_t numValues) const;
    };
}
