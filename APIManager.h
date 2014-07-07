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

#include <unordered_set>
#include <functional>
#include <vector>

#include "ConquerableStationList.h"
#include "APIResponseCache.h"
#include "WalletJournal.h"
#include "APIInterface.h"
#include "AssetList.h"
#include "Character.h"
#include "RefType.h"

namespace Evernus
{
    class APIManager
        : public QObject
    {
        Q_OBJECT

    public:
        template<class T>
        using Callback = std::function<void (const T &data, const QString &error)>;

        typedef std::vector<Character::IdType> CharacterList;
        typedef std::vector<RefType> RefTypeList;

        static const QString eveTimeFormat;

        APIManager();
        virtual ~APIManager() = default;

        void fetchCharacterList(const Key &key, const Callback<CharacterList> &callback) const;
        void fetchCharacter(const Key &key, Character::IdType characterId, const Callback<Character> &callback) const;
        void fetchAssets(const Key &key, Character::IdType characterId, const Callback<AssetList> &callback) const;
        void fetchConquerableStationList(const Callback<ConquerableStationList> &callback) const;
        void fetchRefTypes(const Callback<RefTypeList> &callback) const;
        void fetchWalletJournal(const Key &key,
                                Character::IdType characterId,
                                WalletJournalEntry::IdType fromId,
                                WalletJournalEntry::IdType tillId,
                                const Callback<WalletJournal> &callback) const;

        QDateTime getCharacterLocalCacheTime(Character::IdType characterId) const;
        QDateTime getAssetsLocalCacheTime(Character::IdType characterId) const;
        QDateTime getWalletJournalLocalCacheTime(Character::IdType characterId) const;

    signals:
        void generalError(const QString &info);

    private:
        mutable APIResponseCache mCache;
        APIInterface mInterface;

        mutable std::unordered_set<Key::IdType> mPendingCharacterListRequests;
        mutable std::unordered_set<Character::IdType> mPendingCharacterRequests;
        mutable std::unordered_set<Character::IdType> mPendingAssetsRequests;

        void fetchWalletJournal(const Key &key,
                                Character::IdType characterId,
                                WalletJournalEntry::IdType fromId,
                                WalletJournalEntry::IdType tillId,
                                std::shared_ptr<WalletJournal> &&journal,
                                const Callback<WalletJournal> &callback) const;

        template<class T, class CurElem>
        static std::vector<T> parseResults(const QString &xml, const QString &rowsetName);
        template<class T>
        static T parseResult(const QString &xml);

        static QString queryPath(const QString &path, const QString &xml);

        static QDateTime getCachedUntil(const QString &xml);
        static void handlePotentialError(const QString &xml, const QString &error);
    };
}
