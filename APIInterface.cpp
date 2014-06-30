#include <QUrlQuery>
#include <QSettings>

#include "NetworkSettings.h"
#include "Key.h"

#include "APIInterface.h"

namespace Evernus
{
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
        QString url;

        if (settings.value(NetworkSettings::useCustomProvider).toBool())
            url = settings.value(NetworkSettings::providerHost).toString();
        else
            url = NetworkSettings::defaultAPIProvider;

        QUrlQuery postData;
        postData.addQueryItem("keyID", QString::number(key.getId()));
        postData.addQueryItem("vCode", key.getCode());

        for (const auto &param : additionalParams)
            postData.addQueryItem(param.first, param. second);

        QNetworkRequest request{url + endpoint};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        auto reply = mNetworkManager.post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
        connect(reply, &QNetworkReply::finished, this, &APIInterface::processReply, Qt::QueuedConnection);
        connect(reply, &QNetworkReply::sslErrors, this, &APIInterface::processSslErrors);

        mPendingCallbacks[reply] = callback;
    }
}
