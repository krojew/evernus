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
#include <QCoreApplication>
#include <QUrlQuery>
#include <QtDebug>

#include "NetworkSettings.h"
#include "SecurityHelper.h"
#include "ReplyTimeout.h"
#include "CorpKey.h"
#include "Key.h"

#include "APIInterface.h"

namespace Evernus
{
    const QString APIInterface::rowLimit = "2560";

    APIInterface::APIInterface(QObject *parent)
        : QObject{parent}
        , mCache{new APIResponseCache{this}}
    {
        mNetworkManager.setCache(mCache);
    }

    void APIInterface::fetchCharacterList(const Key &key, const Callback &callback) const
    {
        makeRequest("/account/Characters.xml.aspx", key, callback, getNumRetries());
    }

    void APIInterface::fetchCharacter(const Key &key, Character::IdType characterId, const Callback &callback) const
    {
        makeRequest("/char/CharacterSheet.xml.aspx", key, callback, getNumRetries(), { std::make_pair("characterID", QString::number(characterId)) });
    }

    void APIInterface::fetchAssets(const Key &key, Character::IdType characterId, const Callback &callback) const
    {
        makeRequest("/char/AssetList.xml.aspx", key, callback, getNumRetries(), { std::make_pair("characterID", QString::number(characterId)) });
    }

    void APIInterface::fetchAssets(const CorpKey &key, const Callback &callback) const
    {
        makeRequest("/corp/AssetList.xml.aspx", key, callback, getNumRetries());
    }

    void APIInterface::fetchConquerableStationList(const Callback &callback) const
    {
        makeRequest("/eve/ConquerableStationList.xml.aspx", Key{}, callback, getNumRetries());
    }

    void APIInterface::fetchWalletJournal(const Key &key,
                                          Character::IdType characterId,
                                          WalletJournalEntry::IdType fromId,
                                          int accountKey,
                                          const Callback &callback) const
    {
        QueryParams params{std::make_pair("characterID", QString::number(characterId))};
        params.emplace_back("rowCount", rowLimit);
        params.emplace_back("accountKey", QString::number(accountKey));
        if (fromId != WalletJournalEntry::invalidId)
            params.emplace_back("fromID", QString::number(fromId));

        makeRequest("/char/WalletJournal.xml.aspx", key, callback, getNumRetries(), params);
    }

    void APIInterface::fetchWalletTransactions(const Key &key,
                                               Character::IdType characterId,
                                               WalletTransaction::IdType fromId,
                                               int accountKey,
                                               const Callback &callback) const
    {
        QueryParams params{std::make_pair("characterID", QString::number(characterId))};
        params.emplace_back("rowCount", rowLimit);
        params.emplace_back("accountKey", QString::number(accountKey));
        if (fromId != WalletJournalEntry::invalidId)
            params.emplace_back("fromID", QString::number(fromId));

        makeRequest("/char/WalletTransactions.xml.aspx", key, callback, getNumRetries(), params);
    }

    void APIInterface::fetchMarketOrders(const Key &key, Character::IdType characterId, const Callback &callback) const
    {
        makeRequest("/char/MarketOrders.xml.aspx", key, callback, getNumRetries(), { std::make_pair("characterID", QString::number(characterId)) });
    }

    void APIInterface::fetchContracts(const Key &key, Character::IdType characterId, const Callback &callback) const
    {
        makeRequest("/char/Contracts.xml.aspx", key, callback, getNumRetries(), { std::make_pair("characterID", QString::number(characterId)) });
    }

    void APIInterface::fetchContractItems(const Key &key,
                                          Character::IdType characterId,
                                          Contract::IdType contractId,
                                          const Callback &callback) const
    {
        QueryParams params;
        params.emplace_back(std::make_pair("characterID", QString::number(characterId)));
        params.emplace_back(std::make_pair("contractID", QString::number(contractId)));

        makeRequest("/char/ContractItems.xml.aspx", key, callback, getNumRetries(), params);
    }

    void APIInterface::fetchWalletJournal(const CorpKey &key,
                                          Character::IdType characterId,
                                          WalletJournalEntry::IdType fromId,
                                          int accountKey,
                                          const Callback &callback) const
    {
        QueryParams params{std::make_pair("characterID", QString::number(characterId))};
        params.emplace_back("rowCount", rowLimit);
        params.emplace_back("accountKey", QString::number(accountKey));
        if (fromId != WalletJournalEntry::invalidId)
            params.emplace_back("fromID", QString::number(fromId));

        makeRequest("/corp/WalletJournal.xml.aspx", key, callback, getNumRetries(), params);
    }

    void APIInterface::fetchWalletTransactions(const CorpKey &key,
                                               Character::IdType characterId,
                                               WalletTransaction::IdType fromId,
                                               int accountKey,
                                               const Callback &callback) const
    {
        QueryParams params{std::make_pair("characterID", QString::number(characterId))};
        params.emplace_back("rowCount", rowLimit);
        params.emplace_back("accountKey", QString::number(accountKey));
        if (fromId != WalletJournalEntry::invalidId)
            params.emplace_back("fromID", QString::number(fromId));

        makeRequest("/corp/WalletTransactions.xml.aspx", key, callback, getNumRetries(), params);
    }

    void APIInterface::fetchMarketOrders(const CorpKey &key, Character::IdType characterId, const Callback &callback) const
    {
        makeRequest("/corp/MarketOrders.xml.aspx", key, callback, getNumRetries(), { std::make_pair("characterID", QString::number(characterId)) });
    }

    void APIInterface::fetchContracts(const CorpKey &key, Character::IdType characterId, const Callback &callback) const
    {
        makeRequest("/corp/Contracts.xml.aspx", key, callback, getNumRetries(), { std::make_pair("characterID", QString::number(characterId)) });
    }

    void APIInterface::fetchContractItems(const CorpKey &key,
                                          Character::IdType characterId,
                                          Contract::IdType contractId,
                                          const Callback &callback) const
    {
        QueryParams params;
        params.emplace_back(std::make_pair("characterID", QString::number(characterId)));
        params.emplace_back(std::make_pair("contractID", QString::number(contractId)));

        makeRequest("/corp/ContractItems.xml.aspx", key, callback, getNumRetries(), params);
    }

    void APIInterface::processSslErrors(const QList<QSslError> &errors)
    {
        SecurityHelper::handleSslErrors(errors, *qobject_cast<QNetworkReply *>(sender()));
    }

    void APIInterface::processReply(QNetworkReply *reply) const
    {
        const auto it = mPendingCallbacks.find(reply);
        Q_ASSERT(it != std::end(mPendingCallbacks));

        const auto error = reply->error();

        it->second(reply->readAll(),
                   (error == QNetworkReply::NoError || error == QNetworkReply::ContentAccessDenied) ? (QString{}) : (reply->errorString()));
        mPendingCallbacks.erase(it);
    }

    template<class Key>
    void APIInterface
    ::makeRequest(const QString &endpoint, const Key &key, const Callback &callback, uint retries, const QueryParams &additionalParams) const
    {
        qDebug() << "Making API request:" << endpoint << "retries:" << retries;

        QUrl url;

        if (mSettings.value(NetworkSettings::useCustomProviderKey, NetworkSettings::useCustomProviderDefault).toBool())
            url = mSettings.value(NetworkSettings::providerHostKey).toString() + endpoint;
        else
            url = NetworkSettings::defaultAPIProvider + endpoint;

        QUrlQuery queryData;
        queryData.addQueryItem("keyID", QString::number(key.getId()));
        queryData.addQueryItem("vCode", key.getCode());

        for (const auto &param : additionalParams)
            queryData.addQueryItem(param.first, param. second);

        url.setQuery(queryData);

        QNetworkRequest request{url};
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));

        auto reply = mNetworkManager.get(request);

        new ReplyTimeout{*reply};

        connect(reply, &QNetworkReply::finished, this, [=] {
            reply->deleteLater();

            const auto error = reply->error();
            if (Q_UNLIKELY(error != QNetworkReply::NoError && retries > 0))
            {
                mPendingCallbacks.erase(reply);
                makeRequest(endpoint, key, callback, retries - 1, additionalParams);
            }
            else
            {
                processReply(reply);
            }
        }, Qt::QueuedConnection);
        connect(reply, &QNetworkReply::sslErrors, this, &APIInterface::processSslErrors);

        mPendingCallbacks[reply] = callback;
    }

    uint APIInterface::getNumRetries() const
    {
        return mSettings.value(NetworkSettings::maxRetriesKey, NetworkSettings::maxRetriesDefault).toUInt();
    }
}
