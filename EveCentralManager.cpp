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
#include <unordered_map>

#include <QCoreApplication>
#include <QXmlStreamReader>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QDateTime>
#include <QDebug>
#include <QUrl>

#include "EveDataProvider.h"

#include "EveCentralManager.h"

namespace Evernus
{
    const QString EveCentralManager::baseUrl = "http://api.eve-central.com/api/quicklook";

    EveCentralManager::EveCentralManager(const EveDataProvider &dataProvider, QObject *parent)
        : QObject{parent}
        , mDataProvider{dataProvider}
    {
    }

    size_t EveCentralManager
    ::aggregateAndFetchMarketOrders(const TypeLocationPairs &target, const Callback &callback) const
    {
        std::unordered_multimap<TypeLocationPair::first_type, uint> aggregated;
        for (const auto &pair : target)
        {
            const auto regionId = mDataProvider.getStationRegionId(pair.second);
            if (regionId != 0)
                aggregated.emplace(pair.first, regionId);
        }

        auto counter = 0u;
        for (auto it = std::begin(aggregated); it != std::end(aggregated);)
        {
            const auto end = aggregated.equal_range(it->first).second;
            const auto typeId = it->first;

            QUrlQuery query{"typeid=" + QString::number(typeId)};

            do {
                query.addQueryItem("regionlimit", QString::number(it->second));
            } while (++it != end);

            QUrl url{baseUrl};
            url.setQuery(query);

            ++counter;

            QNetworkRequest request{url};
            request.setHeader(QNetworkRequest::UserAgentHeader,
                              QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));

            makeMarketOrderRequest(typeId, request, callback);
        }

        return counter;
    }

    void EveCentralManager::fetchMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const
    {
        QNetworkRequest request{QUrl{QString{baseUrl + "?typeid=%1&regionlimit=%2"}.arg(typeId).arg(regionId)}};
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));

        makeMarketOrderRequest(typeId, request, callback);
    }

    void EveCentralManager
    ::makeMarketOrderRequest(EveType::IdType typeId, const QNetworkRequest &request, const Callback &callback) const
    {
        qDebug() << "Fetching" << request.url();

        auto reply = mNetworkManager.get(request);
        connect(reply, &QNetworkReply::finished, this, [=] {
            reply->deleteLater();

            const auto replyError = reply->error();
            if (replyError != QNetworkReply::NoError)
                callback(std::vector<ExternalOrder>(), reply->errorString());
            else
                processResult(typeId, reply->readAll(), callback);
        }, Qt::QueuedConnection);
    }

    void EveCentralManager
    ::processResult(ExternalOrder::TypeIdType typeId, const QByteArray &data, const Callback &callback) const
    {
        QXmlStreamReader reader{data};

        std::vector<ExternalOrder> result;
        ExternalOrder order;

        const auto currentDate = QDateTime::currentDateTimeUtc();

        while (!reader.atEnd())
        {
            const auto token = reader.readNext();
            if (token == QXmlStreamReader::StartElement)
            {
                const auto name = reader.name();
                if (name == "sell_orders")
                {
                    order.setType(ExternalOrder::Type::Sell);
                }
                else if (name == "buy_orders")
                {
                    order.setType(ExternalOrder::Type::Buy);
                }
                else if (name == "order")
                {
                    order.setId(reader.attributes().value("id").toULongLong());
                }
                else if (name == "region")
                {
                    order.setRegionId(reader.readElementText().toUInt());
                }
                else if (name == "station")
                {
                    order.setStationId(reader.readElementText().toUInt());
                }
                else if (name == "range")
                {
                    order.setRange(reader.readElementText().toShort());
                }
                else if (name == "price")
                {
                    order.setPrice(reader.readElementText().toDouble());
                }
                else if (name == "vol_remain")
                {
                    const auto volume = reader.readElementText().toUInt();

                    order.setVolumeRemaining(volume);
                    order.setVolumeEntered(volume);
                }
                else if (name == "min_volume")
                {
                    order.setMinVolume(reader.readElementText().toUInt());
                }
                else if (name == "expires")
                {
                    order.setDuration(currentDate.daysTo(QDateTime::fromString(reader.readElementText(), "yyyy-MM-dd")));
                }
                else if (name == "reported_time")
                {
                    const auto dt = QDateTime::fromString(reader.readElementText(), "MM-dd hh:mm:ss");
                    order.setUpdateTime(dt.addYears(currentDate.date().year() - dt.date().year()));
                }
            }
            else if (token == QXmlStreamReader::EndElement && reader.name() == "order")
            {
                if (order.getId() != ExternalOrder::invalidId && order.getUpdateTime().isValid())
                {
                    order.setIssued(order.getUpdateTime());
                    order.setSolarSystemId(mDataProvider.getStationSolarSystemId(order.getStationId()));
                    order.setTypeId(typeId);

                    result.emplace_back(std::move(order));
                    order.setId(ExternalOrder::invalidId);
                }
            }
        }

        if (reader.hasError())
            callback(std::vector<ExternalOrder>(), reader.errorString());
        else
            callback(std::move(result), QString{});
    }
}
