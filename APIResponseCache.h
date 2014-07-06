/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <unordered_map>
#include <memory>
#include <vector>

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

        bool hasCharacterData(Character::IdType characterId) const;
        Character getCharacterData(Character::IdType characterId) const;
        void setCharacterData(Character::IdType characterId, const Character &data, const QDateTime &cacheUntil);

        QDateTime getCharacterDataLocalCacheTime(Character::IdType characterId) const;

        bool hasAssetData(Character::IdType characterId) const;
        AssetList getAssetData(Character::IdType characterId) const;
        void setAssetData(Character::IdType characterId, const AssetList &data, const QDateTime &cacheUntil);

        QDateTime getAssetsDataLocalCacheTime(Character::IdType characterId) const;

        bool hasConquerableStationListData() const;
        ConquerableStationList getConquerableStationListData() const;
        void setConquerableStationListData(const ConquerableStationList &data, const QDateTime &cacheUntil);

        QDateTime getWalletJournalLocalCacheTime(Character::IdType characterId) const;

    private:
        template<class T>
        struct CacheEntry
        {
            QDateTime mCacheUntil;
            T mData;
        };

        mutable std::unordered_map<Key::IdType, CacheEntry<CharacterList>> mCharacterListCache;
        mutable std::unordered_map<Character::IdType, CacheEntry<Character>> mCharacterCache;
        mutable std::unordered_map<Character::IdType, CacheEntry<AssetList>> mAssetCache;
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

        void saveItemTree(const Item &item, const Item *parent, QVariantList boundValues[CachedItemRepository::columnCount - 1]) const;

        QSqlQuery prepareBatchConquerableStationInsertQuery(size_t numValues) const;
        QSqlQuery prepareBatchItemInsertQuery(size_t numValues) const;
    };
}
