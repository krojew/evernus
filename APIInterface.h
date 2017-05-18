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
#include <QSettings>

#include "WalletJournalEntry.h"
#include "WalletTransaction.h"
#include "APIResponseCache.h"
#include "Character.h"
#include "Contract.h"

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
        void fetchAssets(const CorpKey &key, const Callback &callback) const;
        void fetchConquerableStationList(const Callback &callback) const;
        void fetchRefTypes(const Callback &callback) const;
        void fetchWalletJournal(const Key &key,
                                Character::IdType characterId,
                                WalletJournalEntry::IdType fromId,
                                int accountKey,
                                const Callback &callback) const;
        void fetchWalletTransactions(const Key &key,
                                     Character::IdType characterId,
                                     WalletTransaction::IdType fromId,
                                     int accountKey,
                                     const Callback &callback) const;
        void fetchMarketOrders(const Key &key, Character::IdType characterId, const Callback &callback) const;
        void fetchContracts(const Key &key, Character::IdType characterId, const Callback &callback) const;
        void fetchContractItems(const Key &key,
                                Character::IdType characterId,
                                Contract::IdType contractId,
                                const Callback &callback) const;
        void fetchWalletJournal(const CorpKey &key,
                                Character::IdType characterId,
                                WalletJournalEntry::IdType fromId,
                                int accountKey,
                                const Callback &callback) const;
        void fetchWalletTransactions(const CorpKey &key,
                                     Character::IdType characterId,
                                     WalletTransaction::IdType fromId,
                                     int accountKey,
                                     const Callback &callback) const;
        void fetchMarketOrders(const CorpKey &key, Character::IdType characterId, const Callback &callback) const;
        void fetchContracts(const CorpKey &key, Character::IdType characterId, const Callback &callback) const;
        void fetchContractItems(const CorpKey &key,
                                Character::IdType characterId,
                                Contract::IdType contractId,
                                const Callback &callback) const;
        void fetchGenericName(quint64 id, const Callback &callback) const;

    private slots:
        void processSslErrors(const QList<QSslError> &errors);

    private:
        typedef std::vector<std::pair<QString, QString>> QueryParams;

        static const QString rowLimit;

        APIResponseCache *mCache = nullptr;
        mutable QNetworkAccessManager mNetworkManager;

        mutable std::unordered_map<QNetworkReply *, Callback> mPendingCallbacks;

        QSettings mSettings;

        void processReply(QNetworkReply *reply) const;

        template<class Key>
        void makeRequest(const QString &endpoint,
                         const Key &key,
                         const Callback &callback,
                         uint retries,
                         const QueryParams &additionalParams = QueryParams{}) const;

        uint getNumRetries() const;
    };
}
