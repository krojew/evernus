#include <QUrlQuery>
#include <QSettings>

#include "NetworkSettings.h"
#include "Key.h"

#include "APIInterface.h"

namespace Evernus
{
    void APIInterface::fetchCharacterList(const Key &key, const Callback &callback)
    {
        makeRequest("/account/Characters.xml.aspx", key, callback);
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

    void APIInterface::makeRequest(const QString &endpoint, const Key &key, const Callback &callback)
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

        QNetworkRequest request{url + endpoint};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        auto reply = mNetworkManager.post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
        connect(reply, &QNetworkReply::finished, this, &APIInterface::processReply, Qt::QueuedConnection);
        connect(reply, &QNetworkReply::sslErrors, this, &APIInterface::processSslErrors);

        mPendingCallbacks[reply] = callback;
    }
}
