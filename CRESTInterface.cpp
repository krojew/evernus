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
#include <thread>

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QUrlQuery>
#include <QSettings>
#include <QDebug>

#include "SecurityHelper.h"
#include "CRESTSettings.h"
#include "ReplyTimeout.h"
#include "Defines.h"

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

    RateLimiter CRESTInterface::mCRESTLimiter;

    QTimer CRESTInterface::mRequestTimer;

    CRESTInterface::CRESTInterface(QObject *parent)
        : QObject(parent)
    {
        QSettings settings;
        mCRESTLimiter.setRate(settings.value(CRESTSettings::rateLimitKey, CRESTSettings::rateLimitDefault).toFloat());

        if (!mRequestTimer.isActive())
        {
            const auto requestTimerInterval = 2;

            mRequestTimer.setTimerType(Qt::PreciseTimer);
            mRequestTimer.start(requestTimerInterval);
        }

        connect(&mRequestTimer, &QTimer::timeout, this, &CRESTInterface::processPendingRequests);
    }

    void CRESTInterface::fetchMarketOrders(uint regionId, EveType::IdType typeId, const JsonCallback &callback) const
    {
        qDebug() << "Fetching market orders for" << regionId << "and" << typeId;

        auto orderFetcher = [=](const QUrl &url, const QString &error) {
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, error);
                return;
            }

            getRegionData(url, typeId, "application/vnd.ccp.eve.MarketOrderCollection-v1+json", callback);
        };

        getRegionUrl(regionId, mRegionOrdersUrls, mPendingRegionOrdersRequests, "marketOrders", orderFetcher);
    }

    void CRESTInterface::fetchMarketOrders(uint regionId, const PaginatedCallback &callback) const
    {
        qDebug() << "Fetching whole market for" << regionId;

        auto marketFetcher = [=](const QUrl &url, const QString &error) {
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, true, error);
                return;
            }

            fetchPaginatedOrders(callback, url);
        };

        getRegionUrl(regionId, mRegionMarketUrls, mPendingRegionMarketRequests, "marketOrdersAll", marketFetcher);
    }

    void CRESTInterface::fetchMarketHistory(uint regionId, EveType::IdType typeId, const JsonCallback &callback) const
    {
        qDebug() << "Fetching market history for" << regionId << "and" << typeId;

        auto historyFetcher = [=](const auto &url, const auto &error) {
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, error);
                return;
            }

            getRegionData(url, typeId, "application/vnd.ccp.eve.MarketTypeHistoryCollection-v1+json", callback);
        };

        getRegionUrl(regionId, mRegionHistoryUrls, mPendingRegionHistoryRequests, "marketHistory", historyFetcher);
    }

    void CRESTInterface::openMarketDetails(EveType::IdType typeId, Character::IdType charId, const ErrorCallback &errorCallback) const
    {
        qDebug() << "Opening market details for" << typeId;

        if (!mEndpoints.contains(itemTypesUrlName))
        {
            errorCallback(tr("Missing CREST item types url!"));
            return;
        }

        auto opener = [=](const auto &error) {
            if (!error.isEmpty())
            {
                errorCallback(error);
            }
            else
            {
                QJsonDocument json{QJsonObject{
                    { "type", QJsonObject{
                        { "href", QStringLiteral("%1%2/").arg(mEndpoints[itemTypesUrlName]).arg(typeId) },
                        { "id", static_cast<qint64>(typeId) },
                    } }
                }};

                post(QStringLiteral("%1/characters/%2/ui/openwindow/marketdetails/").arg(crestUrl).arg(charId), json.toJson(), std::move(errorCallback));
            }
        };

        checkAuth(opener);
    }

    void CRESTInterface::setEndpoints(EndpointMap endpoints)
    {
        mEndpoints = std::move(endpoints);
    }

    void CRESTInterface::updateTokenAndContinue(QString token, const QDateTime &expiry)
    {
        mAccessToken = std::move(token);
        mExpiry = expiry;

        for (const auto &request : mPendingAuthRequests)
            request(QString{});

        mPendingAuthRequests.clear();
    }

    void CRESTInterface::handleTokenError(const QString &error)
    {
        for (const auto &request : mPendingAuthRequests)
            request(error);

        mPendingAuthRequests.clear();
    }

    void CRESTInterface::processSslErrors(const QList<QSslError> &errors)
    {
        SecurityHelper::handleSslErrors(errors, *qobject_cast<QNetworkReply *>(sender()));
    }

    void CRESTInterface::processPendingRequests()
    {
        auto it = mPendingRequests.begin();
        while (it != std::end(mPendingRequests) && it->first <= std::chrono::steady_clock::now())
        {
            it->second();
            it = mPendingRequests.erase(it);
        }
    }

    void CRESTInterface::setRateLimit(float rate)
    {
        mCRESTLimiter.setRate(rate);
    }

    template<class T>
    void CRESTInterface::checkAuth(T &&continuation) const
    {
        if (mExpiry < QDateTime::currentDateTime() || mAccessToken.isEmpty())
        {
            mPendingAuthRequests.emplace_back(std::forward<T>(continuation));
            if (mPendingAuthRequests.size() == 1)
                emit tokenRequested();
        }
        else
        {
            std::forward<T>(continuation)(QString{});
        }
    }

    template<class T>
    void CRESTInterface::getRegionUrl(uint regionId, RegionUrlMap &urlMap, RegionUrlCallbackMap &callbackMap, const QString &urlName, T &&continuation) const
    {
        if (!mEndpoints.contains(regionsUrlName))
        {
            continuation(QUrl{}, tr("Missing CREST regions url!"));
            return;
        }

        const auto pendingKey = qMakePair(regionId, urlName);
        if (callbackMap.contains(pendingKey))
        {
            callbackMap[pendingKey].emplace_back(std::forward<T>(continuation));
            return;
        }

        qDebug() << "Fetching region url:" << regionId << urlName;

        callbackMap[pendingKey].emplace_back(std::forward<T>(continuation));

        auto notifyError = [=, &callbackMap](const auto &error) {
            for (const auto &continuation : callbackMap[pendingKey])
                continuation(QUrl{}, error);

            callbackMap.remove(pendingKey);
        };

        auto extractUrl = [=, &urlMap, &callbackMap](const auto &doc, const auto &error) {
            if (!error.isEmpty())
            {
                notifyError(error);
                return;
            }

            const QUrl href = doc.object().value(urlName).toObject().value("href").toString();
            urlMap[regionId] = href;

            qDebug() << "Region url:" << href;

            for (const auto &continuation : callbackMap[pendingKey])
                continuation(href, QString{});

            callbackMap.remove(pendingKey);
        };

        if (mRegionUrls.contains(regionId))
        {
            asyncGet(mRegionUrls[regionId], "application/vnd.ccp.eve.Region-v1+json", extractUrl);
        }
        else
        {
            auto saveUrl = [=, &callbackMap](const QJsonDocument &doc, const QString &error) {
                if (!error.isEmpty())
                {
                    notifyError(error);
                    return;
                }

                QUrl href;

                const auto items = doc.object().value("items").toArray();
                for (const auto &item : items)
                {
                    const auto &itemObj = item.toObject();
                    if (itemObj.value("id").toInt() == regionId)
                    {
                        href = itemObj.value("href").toString();
                        break;
                    }
                }

                qDebug() << "Region url:" << href;

                if (href.isEmpty())
                {
                    notifyError(tr("Missing region URL for %1").arg(regionId));
                    return;
                }

                mRegionUrls[regionId] = href;

                asyncGet(href, "application/vnd.ccp.eve.Region-v1+json", extractUrl);
            };

            asyncGet(mEndpoints[regionsUrlName], "application/vnd.ccp.eve.RegionCollection-v1+json", saveUrl);
        }
    }

    template<class T>
    void CRESTInterface::getRegionData(QUrl regionUrl, EveType::IdType typeId, const QByteArray &accept, T &&continuation) const
    {
        if (!mEndpoints.contains(itemTypesUrlName))
        {
            continuation(QJsonDocument{}, tr("Missing CREST item types url!"));
            return;
        }

        QUrlQuery query;
        query.addQueryItem("type", QStringLiteral("%1%2/").arg(mEndpoints[itemTypesUrlName]).arg(typeId));

        regionUrl.setQuery(query);

        asyncGet(regionUrl, accept, std::forward<T>(continuation));
    }

    template<class T>
    void CRESTInterface::getOrders(QUrl regionUrl, T &&continuation) const
    {
        asyncGet(regionUrl, "application/vnd.ccp.eve.MarketOrderCollectionSlim-v1+json", std::forward<T>(continuation));
    }

    void CRESTInterface::fetchPaginatedOrders(const PaginatedCallback &callback, const QUrl &url) const
    {
        auto continuatingCallback = [=](auto &&document, const auto &error) {
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, true, error);
                return;
            }

            const auto object = document.object();
            const auto next = object["next"].toObject()["href"].toString();

            if (!next.isEmpty())
                fetchPaginatedOrders(callback, next);

            callback(std::move(document), next.isEmpty(), QString{});
        };

        getOrders(url, continuatingCallback);
    }

    template<class T>
    void CRESTInterface::asyncGet(const QUrl &url, const QByteArray &accept, T &&continuation) const
    {
        auto request = [=] {
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
                        tryAuthAndContinue([=](const auto &error) {
                            if (error.isEmpty())
                                asyncGet(url, accept, continuation);
                            else
                                continuation(QJsonDocument{}, error);
                        });
                    }
                    else
                    {
                        continuation(QJsonDocument{}, reply->errorString());
                    }
                }
                else
                {
                    continuation(QJsonDocument::fromJson(reply->readAll()), QString{});
                }
            });
        };

        scheduleRequest(request);
    }

    template<class T>
    void CRESTInterface::post(const QUrl &url, const QByteArray &data, T &&errorCallback) const
    {
        auto request = [=] {
            qDebug() << "CREST request:" << url << ":" << data;

            auto request = prepareRequest(url, "application/json");
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

            auto reply = mNetworkManager.post(request, data);
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
                        tryAuthAndContinue([=](const auto &error) {
                            if (error.isEmpty())
                                post(url, data, errorCallback);
                            else
                                errorCallback(error);
                        });
                    }
                    else
                    {
                        errorCallback(reply->errorString());
                    }
                }
                else
                {
                    const auto errorText = reply->readAll();
                    if (!errorText.isEmpty())
                        errorCallback(errorText);
                }
            });
        };

        scheduleRequest(request);
    }

    template<class T>
    void CRESTInterface::scheduleRequest(T &&request) const
    {
        const auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(mCRESTLimiter.acquire());
        if (wait.count() > 0)
            mPendingRequests.emplace(std::chrono::steady_clock::now() + wait, std::move(request));
        else
            request();
    }

    template<class T>
    void CRESTInterface::tryAuthAndContinue(T &&continuation) const
    {
        mAccessToken.clear();
        checkAuth(std::forward<T>(continuation));
    }

    QNetworkRequest CRESTInterface::prepareRequest(const QUrl &url, const QByteArray &accept) const
    {
        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));
        request.setRawHeader("Accept", accept);
        request.setRawHeader("Authorization", "Bearer " + mAccessToken.toLatin1());

        return request;
    }
}
