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
#include <functional>
#include <vector>

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "WalletJournalEntry.h"
#include "WalletTransaction.h"
#include "APIResponseCache.h"
#include "Character.h"

namespace Evernus
{
    class CorpKey;
    class Key;

    class APIInterface
        : public QObject
    {
        Q_OBJECT

    public:
        typedef std::function<void (const QString &response, const QString &error)> Callback;

        APIInterface(QObject *parent = nullptr);
        virtual ~APIInterface() = default;

        void fetchCharacterList(const Key &key, const Callback &callback) const;
        void fetchCharacter(const Key &key, Character::IdType characterId, const Callback &callback) const;
        void fetchAssets(const Key &key, Character::IdType characterId, const Callback &callback) const;
        void fetchConquerableStationList(const Callback &callback) const;
        void fetchRefTypes(const Callback &callback) const;
        void fetchWalletJournal(const Key &key,
                                Character::IdType characterId,
                                WalletJournalEntry::IdType fromId,
                                const Callback &callback) const;
        void fetchWalletTransactions(const Key &key,
                                     Character::IdType characterId,
                                     WalletTransaction::IdType fromId,
                                     const Callback &callback) const;
        void fetchMarketOrders(const Key &key, Character::IdType characterId, const Callback &callback) const;
        void fetchWalletJournal(const CorpKey &key,
                                Character::IdType characterId,
                                WalletJournalEntry::IdType fromId,
                                const Callback &callback) const;
        void fetchWalletTransactions(const CorpKey &key,
                                     Character::IdType characterId,
                                     WalletTransaction::IdType fromId,
                                     const Callback &callback) const;
        void fetchMarketOrders(const CorpKey &key, Character::IdType characterId, const Callback &callback) const;

    signals:
        void generalError(const QString &info);

    private slots:
        void processReply();
        void processSslErrors(const QList<QSslError> &errors);

    private:
        typedef std::vector<std::pair<QString, QString>> QueryParams;

        static const QString rowLimit;

        APIResponseCache *mCache = nullptr;
        mutable QNetworkAccessManager mNetworkManager;

        mutable std::unordered_map<QNetworkReply *, Callback> mPendingCallbacks;

        template<class Key>
        void makeRequest(const QString &endpoint,
                         const Key &key,
                         const Callback &callback,
                         const QueryParams &additionalParams = QueryParams{}) const;
    };
}
