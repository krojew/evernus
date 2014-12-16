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
#include <QDesktopWidget>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QApplication>
#include <QJsonObject>
#include <QWebFrame>
#include <QUrlQuery>
#include <QSettings>
#include <QDebug>

#include <boost/scope_exit.hpp>

#include "PersistentCookieJar.h"
#include "EveDataProvider.h"
#include "CRESTSettings.h"

#include "CRESTExternalOrderImporter.h"

namespace Evernus
{
#ifdef EVERNUS_CREST_SISI
    const QString CRESTExternalOrderImporter::loginUrl = "https://sisilogin.testeveonline.com";
#else
    const QString CRESTExternalOrderImporter::loginUrl = "https://login-tq.eveonline.com";
#endif

    const QString CRESTExternalOrderImporter::redirectUrl = "evernus.com";

    CRESTExternalOrderImporter::CRESTExternalOrderImporter(QByteArray clientId, QByteArray clientSecret, const EveDataProvider &dataProvider, QObject *parent)
        : ExternalOrderImporter{parent}
        , mDataProvider{dataProvider}
        , mClientId{std::move(clientId)}
        , mClientSecret{std::move(clientSecret)}
        , mCrypt{CRESTSettings::cryptKey}
        , mManager{mDataProvider}
    {
        QSettings settings;
        mRefreshToken = mCrypt.decryptToString(settings.value(CRESTSettings::refreshTokenKey).toByteArray());

        connect(&mManager, &CRESTManager::tokenRequested, this, &CRESTExternalOrderImporter::fetchToken);
        connect(this, &CRESTExternalOrderImporter::tokenError, &mManager, &CRESTManager::handleTokenError);
        connect(this, &CRESTExternalOrderImporter::acquiredToken, &mManager, &CRESTManager::updateTokenAndContinue);
    }

    bool CRESTExternalOrderImporter::eventFilter(QObject *watched, QEvent *event)
    {
        Q_ASSERT(event != nullptr);

        if (watched == mAuthView.get() && event->type() == QEvent::Close)
        {
            qDebug() << "Auth window closed.";
            emit tokenError(tr("CREST authorization failed."));
        }

        return QObject::eventFilter(watched, event);
    }

    void CRESTExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        if (!hasClientCredentials())
        {
            emit error(tr("Evernus has been compiled without CREST support. "
                "You can manually specify CREST client id and secret via command line options: --crest-id and --crest-secret"));

            return;
        }

        if (target.empty())
        {
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
            return;
        }

        mResult.clear();

        mPreparingRequests = true;
        BOOST_SCOPE_EXIT(this_) {
            this_->mPreparingRequests = false;
        } BOOST_SCOPE_EXIT_END

        for (const auto &pair : target)
        {
            const auto regionId = mDataProvider.getStationRegionId(pair.second);
            if (regionId != 0)
            {
                ++mRequestCount;
                mManager.fetchMarketOrders(regionId, pair.first, [this](auto &&orders, const auto &error) {
                    processResult(std::move(orders), error);
                });
            }
        }

        qDebug() << "Making" << mRequestCount << "CREST requests...";

        if (mRequestCount == 0)
        {
            emit externalOrdersChanged(mResult);
            mResult.clear();
        }
    }

    void CRESTExternalOrderImporter::fetchToken()
    {
        qDebug() << "Refreshing access token...";

        if (mRefreshToken.isEmpty())
        {
            qDebug() << "No refresh token - requesting access.";

            QUrl url{loginUrl + "/oauth/authorize"};

            QUrlQuery query;
            query.addQueryItem("response_type", "code");
            query.addQueryItem("redirect_uri", "http://" + redirectUrl);
            query.addQueryItem("client_id", mClientId);
            query.addQueryItem("scope", "publicData");

            url.setQuery(query);

            mAuthView = std::make_unique<QWebView>();
            mAuthView->page()->networkAccessManager()->setCookieJar(new PersistentCookieJar{CRESTSettings::cookiesKey});
            mAuthView->setWindowModality(Qt::ApplicationModal);
            mAuthView->setWindowTitle(tr("CREST Authentication"));
            mAuthView->installEventFilter(this);
            mAuthView->adjustSize();
            mAuthView->move(QApplication::desktop()->screenGeometry(QApplication::activeWindow()).center() -
                            mAuthView->rect().center());
            mAuthView->setUrl(url);
            mAuthView->show();

            connect(mAuthView->page()->mainFrame(), &QWebFrame::urlChanged, [=](const QUrl &url) {
                if (url.host() == redirectUrl)
                {
                    mAuthView->removeEventFilter(this);
                    mAuthView->close();

                    qDebug() << "Requesting access token...";

                    QUrlQuery query{url};
                    QByteArray data = "grant_type=authorization_code&code=";
                    data.append(query.queryItemValue("code"));

                    QNetworkRequest request{loginUrl + "/oauth/token"};
                    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
                    request.setRawHeader(
                        "Authorization", (mClientId + ":" + mClientSecret).toBase64());

                    auto reply = mNetworkManager.post(request, data);
                    connect(reply, &QNetworkReply::finished, this, [=] {
                        reply->deleteLater();

                        if (reply->error() != QNetworkReply::NoError)
                        {
                            qDebug() << "Error requesting access token:" << reply->errorString();
                            emit tokenError(reply->errorString());
                            return;
                        }

                        const auto doc = QJsonDocument::fromJson(reply->readAll());
                        const auto object = doc.object();

                        mRefreshToken = object.value("refresh_token").toString();
                        if (mRefreshToken.isEmpty())
                        {
                            qDebug() << "Empty refresh token!";
                            emit tokenError(tr("Empty refresh token!"));
                            return;
                        }

                        QSettings settings;
                        settings.setValue(CRESTSettings::refreshTokenKey, mCrypt.encryptToByteArray(mRefreshToken));

                        emit acquiredToken(object.value("access_token").toString(),
                                           QDateTime::currentDateTime().addSecs(object.value("expires_in").toInt() - 10));
                    });
                }
            });
        }
        else
        {
            qDebug() << "Refreshing token...";

            QByteArray data = "grant_type=refresh_token&refresh_token=";
            data.append(mRefreshToken);

            QNetworkRequest request{loginUrl + "/oauth/token"};
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            request.setRawHeader(
                "Authorization", (mClientId + ":" + mClientSecret).toBase64());

            auto reply = mNetworkManager.post(request, data);
            connect(reply, &QNetworkReply::finished, this, [=] {
                reply->deleteLater();

                const auto doc = QJsonDocument::fromJson(reply->readAll());
                const auto object = doc.object();

                if (reply->error() != QNetworkReply::NoError)
                {
                    qDebug() << "Error refreshing token:" << reply->errorString();

                    if (object.value("error") == "invalid_token")
                    {
                        mRefreshToken.clear();
                        fetchToken();
                    }
                    else
                    {
                        emit tokenError(reply->errorString());
                    }

                    return;
                }

                const auto accessToken = object.value("access_token").toString();
                if (accessToken.isEmpty())
                {
                    qDebug() << "Empty access token!";
                    emit tokenError(tr("Empty access token!"));
                    return;
                }

                emit acquiredToken(accessToken,
                                   QDateTime::currentDateTime().addSecs(doc.object().value("expires_in").toInt() - 10));
            });
        }
    }

    bool CRESTExternalOrderImporter::hasClientCredentials() const
    {
        return !mClientId.isEmpty() && !mClientSecret.isEmpty();
    }

    void CRESTExternalOrderImporter::processResult(std::vector<ExternalOrder> &&orders, const QString &errorText) const
    {
        --mRequestCount;

        qDebug() << "Got reply," << mRequestCount << "remaining.";

        if (!errorText.isEmpty())
        {
            if (mRequestCount == 0)
                mResult.clear();

            emit error(errorText);
            return;
        }

        mResult.reserve(mResult.size() + orders.size());
        mResult.insert(std::end(mResult),
                       std::make_move_iterator(std::begin(orders)),
                       std::make_move_iterator(std::end(orders)));

        if (mRequestCount == 0)
        {
            if (!mPreparingRequests)
            {
                emit externalOrdersChanged(mResult);
                mResult.clear();
            }
        }
        else
        {
            emit statusChanged(tr("CREST import: waiting for %1 server replies").arg(mRequestCount));
        }
    }
}
