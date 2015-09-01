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

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QUrlQuery>
#include <QSettings>
#include <QDebug>

#include "SecurityHelper.h"
#include "ReplyTimeout.h"

#include "CRESTInterface.h"

namespace Evernus
{
#ifdef EVERNUS_CREST_SISI
    const QString CRESTInterface::crestUrl = "https://api-sisi.testeveonline.com";
#else
    const QString CRESTInterface::crestUrl = "https://crest-tq.eveonline.com";
#endif

    const QString CRESTInterface::regionsUrlName = "regions";
    const QString CRESTInterface::itemTypesUrlName = "itemTypes";

    void CRESTInterface::fetchBuyMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const
    {
        qDebug() << "Fetching buy orders for" << regionId << "and" << typeId;

#ifdef Q_OS_WIN
        auto fetcher = [=](const QString &error) {
#else
        auto fetcher = [=, callback = callback](const auto &error) {
#endif
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, error);
                return;
            }

            auto orderFetcher = [=](const QUrl &url, const QString &error) {
                if (!error.isEmpty())
                {
                    callback(QJsonDocument{}, error);
                    return;
                }

                getOrders(url, typeId, callback);
            };

            getRegionBuyOrdersUrl(regionId, orderFetcher);
        };

        checkAuth(fetcher);
    }

    void CRESTInterface::fetchSellMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const
    {
        qDebug() << "Fetching sell orders for" << regionId << "and" << typeId;

#ifdef Q_OS_WIN
        auto fetcher = [=](const QString &error) {
#else
        auto fetcher = [=, callback = callback](const auto &error) {
#endif
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, error);
                return;
            }

            auto orderFetcher = [=](const QUrl &url, const QString &error) {
                if (!error.isEmpty())
                {
                    callback(QJsonDocument{}, error);
                    return;
                }

                getOrders(url, typeId, callback);
            };

            getRegionSellOrdersUrl(regionId, orderFetcher);
        };

        checkAuth(fetcher);
    }

    void CRESTInterface::fetchMarketHistory(uint regionId, EveType::IdType typeId, const Callback &callback) const
    {
        qDebug() << "Fetching market history for" << regionId << "and" << typeId;

        auto fetcher = [=](const QString &error) {
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, error);
                return;
            }

            // TODO: use endpoint map, when available
            asyncGet(QString{"%1/market/%2/types/%3/history/"}.arg(crestUrl).arg(regionId).arg(typeId),
                     "application/vnd.ccp.eve.MarketTypeHistoryCollection-v1+json",
                     callback);
        };

        checkAuth(fetcher);
    }

    void CRESTInterface::setEndpoints(EndpointMap endpoints)
    {
        mEndpoints = std::move(endpoints);
    }

    void CRESTInterface::updateTokenAndContinue(QString token, const QDateTime &expiry)
    {
        mAccessToken = std::move(token);
        mExpiry = expiry;

        if (mEndpoints.isEmpty())
        {
            for (const auto &request : mPendingRequests)
                request(tr("Empty CREST endpoint map. Please wait until endpoints have been fetched."));
        }
        else
        {
            for (const auto &request : mPendingRequests)
                request(QString{});
        }

        mPendingRequests.clear();
    }

    void CRESTInterface::handleTokenError(const QString &error)
    {
        for (const auto &request : mPendingRequests)
            request(error);

        mPendingRequests.clear();
    }

    void CRESTInterface::processSslErrors(const QList<QSslError> &errors)
    {
        SecurityHelper::handleSslErrors(errors, *qobject_cast<QNetworkReply *>(sender()));
    }

    template<class T>
    void CRESTInterface::getRegionBuyOrdersUrl(uint regionId, T &&continuation) const
    {
        getRegionOrdersUrl(regionId, "marketBuyOrders", mRegionBuyOrdersUrls, std::forward<T>(continuation));
    }

    template<class T>
    void CRESTInterface::getRegionSellOrdersUrl(uint regionId, T &&continuation) const
    {
        getRegionOrdersUrl(regionId, "marketSellOrders", mRegionSellOrdersUrls, std::forward<T>(continuation));
    }

    template<class T>
    void CRESTInterface::getRegionOrdersUrl(uint regionId,
                                            const QString &urlName,
                                            RegionOrderUrlMap &map,
                                            T &&continuation) const
    {
        if (map.contains(regionId))
        {
            continuation(map[regionId], QString{});
            return;
        }

        if (!mEndpoints.contains(regionsUrlName))
        {
            continuation(QUrl{}, tr("Missing CREST regions url!"));
            return;
        }

        const auto pendingKey = qMakePair(regionId, urlName);
        if (mPendingRegionRequests.contains(pendingKey))
        {
            mPendingRegionRequests[pendingKey].emplace_back(std::forward<T>(continuation));
            return;
        }

        qDebug() << "Fetching region orders url:" << regionId << urlName;

        mPendingRegionRequests[pendingKey].emplace_back(std::forward<T>(continuation));

        auto saveUrl = [=, &map](const QJsonDocument &doc, const QString &error) {
            if (!error.isEmpty())
            {
                for (const auto &continuation : mPendingRegionRequests[pendingKey])
                    continuation(QUrl{}, error);

                mPendingRegionRequests.remove(pendingKey);
                return;
            }

            const QUrl href = doc.object().value(urlName).toObject().value("href").toString();
            map[regionId] = href;

            qDebug() << "Region orders url:" << href;

            for (const auto &continuation : mPendingRegionRequests[pendingKey])
                continuation(href, QString{});

            mPendingRegionRequests.remove(pendingKey);
        };

        asyncGet(QString{"%1%2/"}.arg(mEndpoints[regionsUrlName]).arg(regionId), "application/vnd.ccp.eve.Region-v1+json", saveUrl);
    }

    template<class T>
    void CRESTInterface::getOrders(QUrl regionUrl, EveType::IdType typeId, T &&continuation) const
    {
        if (!mEndpoints.contains(itemTypesUrlName))
        {
            continuation(QJsonDocument{}, tr("Missing CREST item types url!"));
            return;
        }

        QUrlQuery query;
        query.addQueryItem("type", QString{"%1%2/"}.arg(mEndpoints[itemTypesUrlName]).arg(typeId));

        regionUrl.setQuery(query);

        asyncGet(regionUrl, "application/vnd.ccp.eve.MarketOrderCollection-v1+json", std::forward<T>(continuation));
    }

    template<class T>
    void CRESTInterface::checkAuth(T &&continuation) const
    {
        if (mExpiry < QDateTime::currentDateTime() || mAccessToken.isEmpty())
        {
            mPendingRequests.emplace_back(std::forward<T>(continuation));
            if (mPendingRequests.size() == 1)
                emit tokenRequested();
        }
        else
        {
            continuation(QString{});
        }
    }

    template<class T>
    void CRESTInterface::asyncGet(const QUrl &url, const QByteArray &accept, T &&continuation) const
    {
        qDebug() << "CREST request:" << url;

        auto reply = mNetworkManager.get(prepareRequest(url, accept));
        Q_ASSERT(reply != nullptr);

        new ReplyTimeout{*reply};

        connect(reply, &QNetworkReply::sslErrors, this, &CRESTInterface::processSslErrors);
        connect(reply, &QNetworkReply::finished, this, [=] {
            reply->deleteLater();

            const auto error = reply->error();
            if (error != QNetworkReply::NoError)
            {
                if (error == QNetworkReply::AuthenticationRequiredError)
                {
                    // expired token?
                    mAccessToken.clear();
                    checkAuth([=](const QString &error) {
                        if (error.isEmpty())
                            asyncGet(url, accept, continuation);
                        else
                            continuation(QJsonDocument{}, error);
                    });
                    return;
                }

                continuation(QJsonDocument{}, reply->errorString());
                return;
            }

            continuation(QJsonDocument::fromJson(reply->readAll()), QString{});
        });
    }

    QJsonDocument CRESTInterface::syncGet(const QUrl &url, const QByteArray &accept) const
    {
        qDebug() << "CREST request:" << url;

        auto reply = mNetworkManager.get(prepareRequest(url, accept));
        Q_ASSERT(reply != nullptr);

        new ReplyTimeout{*reply};

        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(reply, &QNetworkReply::sslErrors, this, &CRESTInterface::processSslErrors);

        loop.exec(QEventLoop::ExcludeUserInputEvents);

        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
            throw std::runtime_error{reply->errorString().toStdString()};

        return QJsonDocument::fromJson(reply->readAll());
    }

    QNetworkRequest CRESTInterface::prepareRequest(const QUrl &url, const QByteArray &accept) const
    {
        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));
        request.setRawHeader("Authorization", "Bearer " + mAccessToken.toLatin1());
        request.setRawHeader("Accept", accept);

        return request;
    }
}
