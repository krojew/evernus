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
#include <stdexcept>

#include <QNetworkRequest>
#include <QDesktopWidget>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QApplication>
#include <QJsonObject>
#include <QUrlQuery>
#include <QWebFrame>
#include <QDebug>

#include "CRESTInterface.h"

#define STR_VALUE(s) #s
#define EVERNUS_TEXT(s) STR_VALUE(s)
#define EVERNUS_CREST_CLIENT_ID_TEXT EVERNUS_TEXT(EVERNUS_CREST_CLIENT_ID)
#define EVERNUS_CREST_SECRET_TEXT EVERNUS_TEXT(EVERNUS_CREST_SECRET)

namespace Evernus
{
#ifdef EVERNUS_CREST_SISI
    const QString CRESTInterface::crestUrl = "https://api-sisi.testeveonline.com";
    const QString CRESTInterface::loginUrl = "https://sisilogin.testeveonline.com";
#else
    const QString CRESTInterface::crestUrl = "https://crest-tq.eveonline.com";
    const QString CRESTInterface::loginUrl = "https://login.eveonline.com";
#endif
    const QString CRESTInterface::redirectUrl = "evernus.com";

    const QString CRESTInterface::regionsUrlName = "regions";
    const QString CRESTInterface::itemTypesUrlName = "itemTypes";

    bool CRESTInterface::eventFilter(QObject *watched, QEvent *event)
    {
        Q_ASSERT(event != nullptr);

        if (watched == mAuthView.get() && event->type() == QEvent::Close)
        {
            qDebug() << "Auth window closed.";
            for (const auto &request : mPendingRequests)
                request(tr("CREST authorization failed."));

            mPendingRequests.clear();
        }

        return QObject::eventFilter(watched, event);
    }

    void CRESTInterface::fetchBuyMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const
    {
        qDebug() << "Fetching buy orders for" << regionId << "and" << typeId;

#ifdef Q_OS_WIN
        auto fetcher = [=](const auto &error) {
#else
        auto fetcher = [=, callback = callback](const auto &error) {
#endif
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, error);
                return;
            }

            QJsonDocument doc;

            try
            {
                doc = getOrders(getRegionBuyOrdersUrl(regionId), typeId);
            }
            catch (const std::exception &e)
            {
                callback(QJsonDocument{}, e.what());
                return;
            }

            callback(std::move(doc), QString{});
        };

        checkAuth(fetcher);
    }

    void CRESTInterface::fetchSellMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const
    {
        qDebug() << "Fetching sell orders for" << regionId << "and" << typeId;

#ifdef Q_OS_WIN
        auto fetcher = [=](const auto &error) {
#else
        auto fetcher = [=, callback = callback](const auto &error) {
#endif
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, error);
                return;
            }

            QJsonDocument doc;

            try
            {
                doc = getOrders(getRegionSellOrdersUrl(regionId), typeId);
            }
            catch (const std::exception &e)
            {
                callback(QJsonDocument{}, e.what());
                return;
            }

            callback(std::move(doc), QString{});
        };
    }

    QUrl CRESTInterface::getRegionBuyOrdersUrl(uint regionId) const
    {
        return getRegionOrdersUrl(regionId, "marketBuyOrders", mRegionBuyOrdersUrls);
    }

    QUrl CRESTInterface::getRegionSellOrdersUrl(uint regionId) const
    {
        return getRegionOrdersUrl(regionId, "marketSellOrders", mRegionSellOrdersUrls);
    }

    QUrl CRESTInterface::getRegionOrdersUrl(uint regionId,
                                            const QString &urlName,
                                            RegionOrderUrlMap &map) const
    {
        if (map.contains(regionId))
            return map[regionId];

        if (!mEndpoints.contains(regionsUrlName))
            throw std::runtime_error{tr("Missing CREST regions url!").toStdString()};

        qDebug() << "Fetching region orders url:" << regionId << urlName;

        const auto doc = syncGet(QString{"%1%2/"}.arg(mEndpoints[regionsUrlName]).arg(regionId),
                                 "application/vnd.ccp.eve.Region-v1+json");

        const QUrl href = doc.object().value(urlName).toObject().value("href").toString();
        map[regionId] = href;

        qDebug() << "Region orders url:" << href;

        return href;
    }

    QJsonDocument CRESTInterface::getOrders(QUrl regionUrl, EveType::IdType typeId) const
    {
        if (!mEndpoints.contains(itemTypesUrlName))
            throw std::runtime_error{tr("Missing CREST item types url!").toStdString()};

        QUrlQuery query;
        query.addQueryItem("type", QString{"%1%2/"}.arg(mEndpoints[itemTypesUrlName]).arg(typeId));

        regionUrl.setQuery(query);

        return syncGet(regionUrl, "application/vnd.ccp.eve.MarketOrderCollection-v1+json");
    }

    template<class T>
    void CRESTInterface::checkAuth(const T &continuation) const
    {
        if (mExpiry < QDateTime::currentDateTime())
        {
            mPendingRequests.emplace_back(continuation);
            if (mPendingRequests.size() == 1)
            {
                auto processPending = [=](const auto &error) {
                    if (mEndpoints.isEmpty())
                    {
                        qDebug() << "Fetching CREST endpoints...";

                        const auto json = syncGet(crestUrl, "application/vnd.ccp.eve.Api-v3+json");
                        std::function<void (const QJsonObject &)> addEndpoints = [this, &addEndpoints](const QJsonObject &object) {
                            for (auto it = std::begin(object); it != std::end(object); ++it)
                            {
                                const auto value = it.value().toObject();
                                if (value.contains("href"))
                                {
                                    qDebug() << "Endpoint:" << it.key() << "->" << it.value();
                                    mEndpoints[it.key()] = value.value("href").toString();
                                }
                                else
                                {
                                    addEndpoints(value);
                                }
                            }
                        };

                        addEndpoints(json.object());
                    }

                    for (const auto &request : mPendingRequests)
                        request(error);

                    mPendingRequests.clear();
                };

                fetchAccessToken(processPending);
            }
        }
        else
        {
            continuation(QString{});
        }
    }

    template<class T>
    void CRESTInterface::fetchAccessToken(const T &continuation) const
    {
        qDebug() << "Refreshing access token...";

        if (mRefreshToken.isEmpty())
        {
            qDebug() << "No refresh token - requesting access.";

            QUrl url{loginUrl + "/oauth/authorize"};

            QUrlQuery query;
            query.addQueryItem("response_type", "code");
            query.addQueryItem("redirect_uri", "http://" + redirectUrl);
            query.addQueryItem("client_id", EVERNUS_CREST_CLIENT_ID_TEXT);
            query.addQueryItem("scope", "publicData");

            url.setQuery(query);

            mAuthView = std::make_unique<QWebView>();
            mAuthView->setWindowModality(Qt::ApplicationModal);
            mAuthView->setWindowTitle(tr("CREST Authentication"));
            mAuthView->installEventFilter(const_cast<CRESTInterface *>(this));
            mAuthView->adjustSize();
            mAuthView->move(QApplication::desktop()->screenGeometry(QApplication::activeWindow()).center() -
                            mAuthView->rect().center());
            mAuthView->setUrl(url);
            mAuthView->show();

            connect(mAuthView->page()->mainFrame(), &QWebFrame::urlChanged, [=](const auto &url) {
                if (url.host() == redirectUrl)
                {
                    mAuthView->removeEventFilter(const_cast<CRESTInterface *>(this));
                    mAuthView->close();

                    qDebug() << "Requesting access token...";

                    QUrlQuery query{url};
                    QByteArray data = "grant_type=authorization_code&code=";
                    data.append(query.queryItemValue("code"));

                    QNetworkRequest request{loginUrl + "/oauth/token"};
                    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
                    request.setRawHeader(
                        "Authorization", QByteArray{EVERNUS_CREST_CLIENT_ID_TEXT ":" EVERNUS_CREST_SECRET_TEXT}.toBase64());

                    auto reply = mNetworkManager.post(request, data);
                    connect(reply, &QNetworkReply::finished, this, [=] {
                        reply->deleteLater();

                        if (reply->error() != QNetworkReply::NoError)
                        {
                            qDebug() << "Error requesting access token:" << reply->errorString();
                            continuation(reply->errorString());
                            return;
                        }

                        const auto doc = QJsonDocument::fromJson(reply->readAll());
                        const auto object = doc.object();

                        mRefreshToken = object.value("refresh_token").toString();
                        if (mRefreshToken.isEmpty())
                        {
                            qDebug() << "Empty refresh token!";
                            continuation(tr("Empty refresh token!"));
                            return;
                        }

                        mExpiry = QDateTime::currentDateTime().addSecs(object.value("expires_in").toInt() - 10);
                        mAccessToken = object.value("access_token").toString();

                        continuation(QString{});
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
                "Authorization", QByteArray{EVERNUS_CREST_CLIENT_ID_TEXT ":" EVERNUS_CREST_SECRET_TEXT}.toBase64());

            auto reply = mNetworkManager.post(request, data);
            connect(reply, &QNetworkReply::finished, this, [=] {
                reply->deleteLater();

                if (reply->error() != QNetworkReply::NoError)
                {
                    qDebug() << "Error refreshing token:" << reply->errorString();
                    continuation(reply->errorString());
                    return;
                }

                const auto doc = QJsonDocument::fromJson(reply->readAll());
                const auto object = doc.object();

                mAccessToken = object.value("access_token").toString();
                if (mAccessToken.isEmpty())
                {
                    qDebug() << "Empty access token!";
                    continuation(tr("Empty access token!"));
                    return;
                }

                mExpiry = QDateTime::currentDateTime().addSecs(doc.object().value("expires_in").toInt() - 10);

                continuation(QString{});
            });
        }
    }

    QJsonDocument CRESTInterface::syncGet(const QUrl &url, const QByteArray &accept) const
    {
        qDebug() << "CREST request:" << url;

        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));
        request.setRawHeader("Authorization", "Bearer " + mAccessToken.toLatin1());
        request.setRawHeader("Accept", accept);

        auto reply = mNetworkManager.get(request);

        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

        loop.exec(QEventLoop::ExcludeUserInputEvents);

        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
            throw std::runtime_error{reply->errorString().toStdString()};

        return QJsonDocument::fromJson(reply->readAll());
    }
}
