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
#include <QtDebug>

#include <QOAuth2AuthorizationCodeFlow>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QSettings>
#include <QUrlQuery>
#include <QUrl>

#include "ESIOAuthReplyHandler.h"
#include "SecurityHelper.h"
#include "ReplyTimeout.h"
#include "SSOSettings.h"

#include "ESIOAuth.h"

namespace Evernus
{
    ESIOAuth::PendingCallbacks::PendingCallbacks(std::function<void ()> requestCallback, AuthErrorCallback errorCallback)
        : mRequestCallback{std::move(requestCallback)}
        , mErrorCallback{std::move(errorCallback)}
    {
    }

    ESIOAuth::ESIOAuth(QString clientId, QString clientSecret, QObject *parent)
        : QObject{parent}
        , mClientId{std::move(clientId)}
        , mClientSecret{std::move(clientSecret)}
        , mCrypt{SSOSettings::cryptKey}
    {
        QSettings settings;
        settings.beginGroup(SSOSettings::refreshTokenGroup);

        const auto keys = settings.childKeys();
        for (const auto &key : keys)
            mRefreshTokens[key.toULongLong()] = mCrypt.decryptToString(settings.value(key).toByteArray());

        settings.endGroup();
    }

    void ESIOAuth::get(Character::IdType charId, const QUrl &url, QVariantMap parameters, NetworkReplyCallback callback, AuthErrorCallback errorCallback)
    {
        prepareParameters(parameters);
        makeRequest(charId, url, std::move(callback), std::move(errorCallback), [=, parameters = std::move(parameters)] {
            return getOAuth(charId).get(url, parameters);
        });
    }

    QNetworkReply *ESIOAuth::get(const QUrl &url, QVariantMap parameters)
    {
        prepareParameters(parameters);

        const auto reply = mUnauthNetworkAccessManager.get(prepareRequest(url));
        connect(reply, &QNetworkReply::sslErrors, this, &ESIOAuth::processSslErrors);

        new ReplyTimeout{*reply};

        return reply;
    }

    QNetworkReply *ESIOAuth::post(QUrl url, const QVariant &data)
    {
        prepareUrl(url);

        auto request = prepareRequest(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

        const auto reply = mUnauthNetworkAccessManager.post(request, QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::sslErrors, this, &ESIOAuth::processSslErrors);

        new ReplyTimeout{*reply};

        return reply;
    }

    void ESIOAuth::clearRefreshTokens()
    {
        mRefreshTokens.clear();
        for (const auto &oauth : mCharactersOAuths)
            oauth.second->setRefreshToken({});
    }

    void ESIOAuth::cancelSsoAuth(Character::IdType charId)
    {
        processPendingRequests(charId, tr("Authentication cancelled."));
    }

    void ESIOAuth::post(Character::IdType charId, QUrl url, const QVariant &data, NetworkReplyCallback callback, AuthErrorCallback errorCallback)
    {
        prepareUrl(url);
        makeRequest(charId, url, std::move(callback), std::move(errorCallback), [=, data = std::move(data)] {
            const auto &oauth = getOAuth(charId);

            // done manually because https://bugreports.qt.io/browse/QTBUG-65558

            QNetworkRequest request{url};
            request.setHeader(QNetworkRequest::UserAgentHeader, getUserAgent());
            request.setRawHeader("Authorization", QStringLiteral("Bearer: %1").arg(oauth.token()).toUtf8());

            return oauth.networkAccessManager()->post(request, QJsonDocument::fromVariant(data).toJson());
        });
    }

    void ESIOAuth::processSslErrors(const QList<QSslError> &errors)
    {
        SecurityHelper::handleSslErrors(errors, *qobject_cast<QNetworkReply *>(sender()));
    }

    QOAuth2AuthorizationCodeFlow &ESIOAuth::getOAuth(Character::IdType charId)
    {
        auto it = mCharactersOAuths.find(charId);
        if (it == std::end(mCharactersOAuths))
        {
            it = mCharactersOAuths.emplace(charId, new QOAuth2AuthorizationCodeFlow{
                mClientId,
                QStringLiteral("https://login.eveonline.com/oauth/authorize"),
                QStringLiteral("https://login.eveonline.com/oauth/token"),
                new ESINetworkAccessManager{mClientId, mClientSecret, this},
                this
            }).first;
            it->second->setUserAgent(getUserAgent());
            it->second->setClientIdentifierSharedKey(mClientSecret);
            it->second->setScope(QStringLiteral(
                "esi-skills.read_skills.v1 "
                "esi-wallet.read_character_wallet.v1 "
                "esi-assets.read_assets.v1 "
                "esi-ui.open_window.v1 "
                "esi-ui.write_waypoint.v1 "
                "esi-markets.structure_markets.v1 "
                "esi-markets.read_character_orders.v1 "
                "esi-characters.read_blueprints.v1 "
                "esi-contracts.read_character_contracts.v1 "
                "esi-wallet.read_corporation_wallets.v1 "
                "esi-assets.read_corporation_assets.v1 "
                "esi-corporations.read_blueprints.v1 "
                "esi-contracts.read_corporation_contracts.v1 "
                "esi-markets.read_corporation_orders.v1 "
                "esi-industry.read_character_mining.v1 "
                "esi-industry.read_corporation_mining.v1"
            ));
            it->second->setContentType(QAbstractOAuth::ContentType::Json);
            it->second->setRefreshToken(mRefreshTokens[charId]);

            const auto replyHandler = new ESIOAuthReplyHandler{it->second};
            it->second->setReplyHandler(replyHandler);
            connect(replyHandler, &ESIOAuthReplyHandler::error, this, [=](const auto &error) {
                processPendingRequests(charId, error);
            });

            connect(it->second, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
                    this, [=](const auto &url) {
                emit ssoAuthRequested(charId, url);
            });
            connect(it->second, &QOAuth2AuthorizationCodeFlow::granted,
                    this, [=] {
                processPendingRequests(charId);
            });

            // we need a hack, because of https://bugreports.qt.io/browse/QTBUG-65778
            // TODO: remove
            if (!it->second->refreshToken().isEmpty())
            {
                it->second->blockSignals(true);
                it->second->setAccessTokenUrl(QStringLiteral("dummy"));
                it->second->grant();
                it->second->blockSignals(false);
                it->second->authorizationCallbackReceived({
                    { QStringLiteral("code"), QStringLiteral("dummy") },
                    { QStringLiteral("state"), it->second->state() }
                });
                it->second->setAccessTokenUrl(QStringLiteral("https://login.eveonline.com/oauth/token"));
                it->second->refreshAccessToken();
            }
        }

        return *it->second;
    }

    void ESIOAuth::prepareParameters(QVariantMap &parameters)
    {
#ifdef EVERNUS_ESI_SISI
        parameters[QStringLiteral("datasource")] = QStringLiteral("singularity");
#else
        parameters[QStringLiteral("datasource")] = QStringLiteral("tranquility");
#endif
    }

    void ESIOAuth::prepareUrl(QUrl &url)
    {
        QUrlQuery query;
#ifdef EVERNUS_ESI_SISI
        query.addQueryItem(QStringLiteral("datasource"), QStringLiteral("singularity"));
#else
        query.addQueryItem(QStringLiteral("datasource"), QStringLiteral("tranquility"));
#endif

        url.setQuery(query);
    }

    template<class T>
    void ESIOAuth::makeRequest(Character::IdType charId, const QUrl &url, NetworkReplyCallback callback, AuthErrorCallback errorCallback, T replyCreator)
    {
        auto &auth = getOAuth(charId);
        const auto status = auth.status();

        qDebug() << "ESI OAuth:" << charId << url << static_cast<int>(status);

        if (status == QAbstractOAuth::Status::Granted)
        {
            const auto reply = replyCreator();
            Q_ASSERT(reply != nullptr);

            new ReplyTimeout{*reply};

            connect(reply, &QNetworkReply::sslErrors, this, &ESIOAuth::processSslErrors);
            connect(reply, &QNetworkReply::finished, this, [=, &auth, callback = std::move(callback), errorCallback = std::move(errorCallback)] {
                reply->deleteLater();

                const auto error = reply->error();

                qDebug() << "ESI request:" << url << error;

                if (error == QNetworkReply::AuthenticationRequiredError || error == QNetworkReply::ContentAccessDenied)
                {
                    queueRequest(charId, url, std::move(callback), std::move(errorCallback), std::move(replyCreator));
                    if (mPendingRequests[charId].size() == 1)
                        grantOrRefresh(auth);
                }
                else
                {
                    callback(*reply);
                }
            });
        }
        else
        {
            queueRequest(charId, url, std::move(callback), std::move(errorCallback), std::move(replyCreator));

            if (status == QAbstractOAuth::Status::NotAuthenticated)
                grantOrRefresh(auth);
        }
    }

    template<class T>
    void ESIOAuth::queueRequest(Character::IdType charId, const QUrl &url, NetworkReplyCallback callback, AuthErrorCallback errorCallback, T replyCreator)
    {
        mPendingRequests[charId].emplace_back([=, callback = std::move(callback), replyCreator = std::move(replyCreator)] {
            makeRequest(charId, url, std::move(callback), std::move(errorCallback), replyCreator);
        }, errorCallback);
    }

    void ESIOAuth::processPendingRequests(Character::IdType charId)
    {
        const auto requests = std::move(mPendingRequests[charId]);
        for (const auto &request : requests)
            request.mRequestCallback();
    }

    void ESIOAuth::processPendingRequests(Character::IdType charId, const QString &error)
    {
        const auto requests = std::move(mPendingRequests[charId]);
        for (const auto &request : requests)
            request.mErrorCallback(error);
    }

    QNetworkRequest ESIOAuth::prepareRequest(const QUrl &url)
    {
        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::UserAgentHeader, getUserAgent());

        return request;
    }

    QString ESIOAuth::getUserAgent()
    {
        return QStringLiteral("%1 %2").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion());
    }

    void ESIOAuth::grantOrRefresh(QOAuth2AuthorizationCodeFlow &oauth)
    {
        if (oauth.refreshToken().isEmpty())
            oauth.grant();
        else
            oauth.refreshAccessToken();
    }
}
