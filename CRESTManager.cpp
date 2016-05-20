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
#include <QJsonArray>
#include <QByteArray>
#include <QUrlQuery>
#include <QSettings>
#include <QDebug>

#include "EveDataProvider.h"
#include "ExternalOrder.h"
#include "CRESTSettings.h"
#include "ReplyTimeout.h"
#include "Defines.h"

#include "CRESTManager.h"

namespace Evernus
{
    CRESTManager
    ::CRESTManager(const EveDataProvider &dataProvider, QObject *parent)
        : QObject{parent}
        , mDataProvider{dataProvider}
    {
        handleNewPreferences();
        fetchEndpoints();

        connect(&mEndpointTimer, &QTimer::timeout, this, [=] {
            if (hasEndpoints())
                mEndpointTimer.stop();
            else
                fetchEndpoints();
        });
        mEndpointTimer.start(10 * 1000);
    }

    void CRESTManager::fetchMarketOrders(uint regionId,
                                         EveType::IdType typeId,
                                         const Callback<std::vector<ExternalOrder>> &callback) const
    {
        if (!hasEndpoints())
        {
            callback(std::vector<ExternalOrder>(), getMissingEnpointsError());
            return;
        }

        auto ifaceCallback = [=](QJsonDocument &&data, const QString &error) {
            if (!error.isEmpty())
            {
                callback(std::vector<ExternalOrder>(), error);
                return;
            }

            const auto items = data.object().value("items").toArray();

            std::vector<ExternalOrder> orders;
            orders.reserve(items.size());

            for (const auto &item : items)
                orders.emplace_back(getOrderFromJson(item.toObject(), regionId));

            callback(std::move(orders), QString{});
        };

        mInterface.fetchMarketOrders(regionId, typeId, ifaceCallback);
    }

    void CRESTManager::fetchMarketHistory(uint regionId,
                                          EveType::IdType typeId,
                                          const Callback<std::map<QDate, MarketHistoryEntry>> &callback) const
    {
        if (!hasEndpoints())
        {
            callback(std::map<QDate, MarketHistoryEntry>(), getMissingEnpointsError());
            return;
        }

#if EVERNUS_CLANG_LAMBDA_CAPTURE_BUG
        mInterface.fetchMarketHistory(regionId, typeId, [=, callback = callback](QJsonDocument &&data, const QString &error) {
#else
        mInterface.fetchMarketHistory(regionId, typeId, [=](QJsonDocument &&data, const QString &error) {
#endif
            if (!error.isEmpty())
            {
                callback(std::map<QDate, MarketHistoryEntry>(), error);
                return;
            }

            std::map<QDate, MarketHistoryEntry> history;

            const auto items = data.object().value("items").toArray();
            for (const auto &item : items)
            {
                const auto itemObject = item.toObject();
                auto date = QDate::fromString(itemObject.value("date").toString(), Qt::ISODate);

                MarketHistoryEntry entry;
                entry.mAvgPrice = itemObject.value("avgPrice").toDouble();
                entry.mHighPrice = itemObject.value("highPrice").toDouble();
                entry.mLowPrice = itemObject.value("lowPrice").toDouble();
                entry.mOrders = itemObject.value("orderCount").toInt();
                entry.mVolume = itemObject.value("volume_str").toString().toULongLong();

                history.emplace(std::move(date), std::move(entry));
            }

            callback(std::move(history), QString{});
        });
    }

    void CRESTManager::fetchMarketOrders(uint regionId, const SingleItemCallback<ExternalOrder> &callback) const
    {
        if (!hasEndpoints())
        {
            callback(ExternalOrder{}, true, getMissingEnpointsError());
            return;
        }

        mInterface.fetchMarketOrders(regionId, [=](auto &&data, auto atEnd, const auto &error) {
            if (!error.isEmpty())
            {
                callback(ExternalOrder{}, true, error);
                return;
            }

            const auto items = data.object().value("items").toArray();
            const auto size = items.size();
            for (auto i = 0; i < size; ++i)
                callback(getOrderFromJson(items[i].toObject(), regionId), atEnd && i == size - 1, QString{});
        });
    }

    void CRESTManager::handleNewPreferences()
    {
        QSettings settings;

        const auto rate = settings.value(CRESTSettings::rateLimitKey, CRESTSettings::rateLimitDefault).toFloat();
        CRESTInterface::setRateLimit(rate);
    }

    void CRESTManager::fetchEndpoints()
    {
        qDebug() << "Fetching CREST endpoints:" << CRESTInterface::crestUrl;

        QNetworkRequest request{CRESTInterface::crestUrl};
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));
        request.setRawHeader("Accept", "application/vnd.ccp.eve.Api-v3+json");

        auto reply = mNetworkManager.get(request);
        Q_ASSERT(reply != nullptr);

        new ReplyTimeout{*reply};

        connect(reply, &QNetworkReply::finished, this, [=] {
            reply->deleteLater();

            const auto errorCode = reply->error();
            qDebug() << "Got CREST endpoints: " << errorCode;

            if (errorCode != QNetworkReply::NoError)
            {
                emit error(tr("Error fetching CREST endpoints!"));
                return;
            }

            const auto json = QJsonDocument::fromJson(reply->readAll());

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

            addEndpoints(json.object());

            mInterface.setEndpoints(mEndpoints);
        });
    }

    bool CRESTManager::hasEndpoints() const
    {
        return !mEndpoints.isEmpty();
    }

    ExternalOrder CRESTManager::getOrderFromJson(const QJsonObject &object, uint regionId) const
    {
        const auto location = object.value("location").toObject();
        const auto range = object.value("range").toString();

        auto issued = QDateTime::fromString(object.value("issued").toString(), Qt::ISODate);
        issued.setTimeSpec(Qt::UTC);

        ExternalOrder order;

        order.setId(object.value("id_str").toString().toULongLong());
        order.setType((object.value("buy").toBool()) ? (ExternalOrder::Type::Buy) : (ExternalOrder::Type::Sell));
        order.setTypeId(object.value("type").toString().toUInt());
        order.setStationId(location.value("id_str").toString().toUInt());
        //TODO: replace when available
        order.setSolarSystemId(mDataProvider.getStationSolarSystemId(order.getStationId()));
        order.setRegionId(regionId);

        if (range == "station")
            order.setRange(-1);
        else if (range == "system")
            order.setRange(0);
        else if (range == "region")
            order.setRange(32767);
        else
            order.setRange(range.toShort());

        order.setUpdateTime(QDateTime::currentDateTimeUtc());
        order.setPrice(object.value("price").toDouble());
        order.setVolumeEntered(object.value("volumeEntered").toInt());
        order.setVolumeRemaining(object.value("volume").toInt());
        order.setMinVolume(object.value("minVolume").toInt());
        order.setIssued(issued);
        order.setDuration(object.value("duration").toInt());

        return order;
    }

    QString CRESTManager::getMissingEnpointsError()
    {
        return tr("CREST endpoint map is empty. Please wait a while.");
    }
}
