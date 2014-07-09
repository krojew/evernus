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
#include <QUrlQuery>
#include <QSettings>

#include "NetworkSettings.h"
#include "Key.h"

#include "APIInterface.h"

namespace Evernus
{
    APIInterface::APIInterface(QObject *parent)
        : QObject{parent}
        , mCache{new APIResponseCache{this}}
    {
        mNetworkManager.setCache(mCache);
    }

    void APIInterface::fetchCharacterList(const Key &key, const Callback &callback) const
    {
        makeRequest("/account/Characters.xml.aspx", key, callback);
    }

    void APIInterface::fetchCharacter(const Key &key, Character::IdType characterId, const Callback &callback) const
    {
        makeRequest("/char/CharacterSheet.xml.aspx", key, callback, { std::make_pair("characterId", QString::number(characterId)) });
    }

    void APIInterface::fetchAssets(const Key &key, Character::IdType characterId, const Callback &callback) const
    {
        makeRequest("/char/AssetList.xml.aspx", key, callback, { std::make_pair("characterId", QString::number(characterId)) });
    }

    void APIInterface::fetchConquerableStationList(const Callback &callback) const
    {
        makeRequest("/eve/ConquerableStationList.xml.aspx", Key{}, callback);
    }

    void APIInterface::fetchRefTypes(const Callback &callback) const
    {
        makeRequest("/eve/RefTypes.xml.aspx", Key{}, callback);
    }

    void APIInterface::fetchWalletJournal(const Key &key,
                                          Character::IdType characterId,
                                          WalletJournalEntry::IdType fromId,
                                          const Callback &callback) const
    {
        QueryParams params{std::make_pair("characterId", QString::number(characterId))};
        params.emplace_back("rowCount", "2560");
        if (fromId != WalletJournalEntry::invalidId)
            params.emplace_back("fromID", QString::number(fromId));

        makeRequest("/char/WalletJournal.xml.aspx", key, callback, params);
    }

    void APIInterface::processReply()
    {
        auto reply = qobject_cast<QNetworkReply *>(sender());

        const auto it = mPendingCallbacks.find(reply);
        Q_ASSERT(it != std::end(mPendingCallbacks));

        const auto error = reply->error();

        it->second(reply->readAll(),
                   (error == QNetworkReply::NoError || error == QNetworkReply::ContentAccessDenied) ? (QString{}) : (reply->errorString()));
        mPendingCallbacks.erase(it);

        reply->deleteLater();
    }

    void APIInterface::processSslErrors(const QList<QSslError> &errors)
    {
        QStringList errorTexts;
        for (const auto &error : errors)
            errorTexts << error.errorString();

        emit generalError(QString{tr("Encountered SSL errors:\n\n%1")}.arg(errorTexts.join("\n")));
    }

    void APIInterface::makeRequest(const QString &endpoint, const Key &key, const Callback &callback, const QueryParams &additionalParams) const
    {
        QSettings settings;
        QUrl url;

        if (settings.value(NetworkSettings::useCustomProvider).toBool())
            url = settings.value(NetworkSettings::providerHost).toString() + endpoint;
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

        auto reply = mNetworkManager.get(request);
        connect(reply, &QNetworkReply::finished, this, &APIInterface::processReply, Qt::QueuedConnection);
        connect(reply, &QNetworkReply::sslErrors, this, &APIInterface::processSslErrors);

        mPendingCallbacks[reply] = callback;
    }
}
