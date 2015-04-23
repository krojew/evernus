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
#include <QMessageBox>
#include <QUrlQuery>
#include <QSettings>
#include <QDebug>
#include <QTimer>

#include "CRESTSettings.h"
#include "ReplyTimeout.h"

#include "CRESTInterface.h"

namespace Evernus
{
#ifdef EVERNUS_CREST_SISI
    const QString CRESTInterface::crestUrl = "https://public-crest-sisi.testeveonline.com";
#else
    const QString CRESTInterface::crestUrl = "https://public-crest.eveonline.com";
#endif

    const QString CRESTInterface::regionsUrlName = "regions";
    const QString CRESTInterface::itemTypesUrlName = "itemTypes";

    RateLimiter CRESTInterface::mCRESTLimiter;
    QHash<QString, QString> CRESTInterface::mEndpoints;

    CRESTInterface::CRESTInterface(QObject *parent)
        : QObject{parent}
    {
        QSettings settings;
        mCRESTLimiter.setRate(settings.value(CRESTSettings::rateLimitKey, CRESTSettings::rateLimitDefault).toFloat());

        if (!mEndpoints.isEmpty())
            return;

        qDebug() << "Fetching CREST endpoints...";

        asyncGet(crestUrl, "application/vnd.ccp.eve.Api-v3+json", [](const auto &doc, const auto &error) {
            if (!mEndpoints.isEmpty())
                return;

            if (!error.isEmpty())
            {
                qDebug() << "CREST endpoint error:" << error;

                mEndpoints[""] = QString{}; // prevent multiple warnings

                QMessageBox::warning(nullptr, tr("Error"), QString{"CREST error: %1\nCREST will not be available."}.arg(error));
                return;
            }

            std::function<void (const QJsonObject &)> addEndpoints = [=, &addEndpoints](const QJsonObject &object) {
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

            addEndpoints(doc.object());
        });
    }

    void CRESTInterface::fetchBuyMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const
    {
        qDebug() << "Fetching buy orders for" << regionId << "and" << typeId;

        auto orderFetcher = [=](const QUrl &url, const QString &error) {
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, error);
                return;
            }

            getOrders(url, typeId, callback);
        };

        getRegionBuyOrdersUrl(regionId, orderFetcher);
    }

    void CRESTInterface::fetchSellMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const
    {
        qDebug() << "Fetching sell orders for" << regionId << "and" << typeId;

        auto orderFetcher = [=](const QUrl &url, const QString &error) {
            if (!error.isEmpty())
            {
                callback(QJsonDocument{}, error);
                return;
            }

            getOrders(url, typeId, callback);
        };

        getRegionSellOrdersUrl(regionId, orderFetcher);
    }

    void CRESTInterface::fetchMarketHistory(uint regionId, EveType::IdType typeId, const Callback &callback) const
    {
        qDebug() << "Fetching market history for" << regionId << "and" << typeId;

        // TODO: use endpoint map, when available
        asyncGet(QString{"%1/market/%2/types/%3/history/"}.arg(crestUrl).arg(regionId).arg(typeId),
                 "application/vnd.ccp.eve.MarketTypeHistoryCollection-v1+json",
                 callback);
    }

    void CRESTInterface::setRateLimit(float rate)
    {
        mCRESTLimiter.setRate(rate);
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
    void CRESTInterface::asyncGet(const QUrl &url, const QByteArray &accept, T &&continuation) const
    {
        qDebug() << "CREST request:" << url;

        const auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(mCRESTLimiter.acquire());
        QTimer::singleShot(wait.count(), [=] {
            auto reply = mNetworkManager.get(prepareRequest(url, accept));
            Q_ASSERT(reply != nullptr);

            new ReplyTimeout{*reply};

            connect(reply, &QNetworkReply::finished, this, [=] {
                reply->deleteLater();

                const auto error = reply->error();
                if (error != QNetworkReply::NoError)
                    continuation(QJsonDocument{}, reply->errorString());
                else
                    continuation(QJsonDocument::fromJson(reply->readAll()), QString{});
            });
        });
    }

    QNetworkRequest CRESTInterface::prepareRequest(const QUrl &url, const QByteArray &accept) const
    {
        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));
        request.setRawHeader("Accept", accept);

        return request;
    }
}
