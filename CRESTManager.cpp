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
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
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
    namespace
    {
        struct OrderState
        {
            enum class State
            {
                Empty,
                GotResponse,
            } mState = State::Empty;

            std::vector<ExternalOrder> mOrders;
            QString mError;
        };
    }

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

        auto state = std::make_shared<OrderState>();
        auto ifaceCallback = [=](QJsonDocument &&data, const QString &error) {
            if (!error.isEmpty())
            {
                if (!state->mError.isEmpty())
                    callback(std::vector<ExternalOrder>(), QString{"%1\n%2"}.arg(state->mError).arg(error));
                else if (state->mState == OrderState::State::GotResponse)
                    callback(std::vector<ExternalOrder>(), error);
                else
                    state->mError = error;

                return;
            }

            if (!state->mError.isEmpty())
            {
                callback(std::vector<ExternalOrder>(), state->mError);
                return;
            }

            const auto items = data.object().value("items").toArray();
            state->mOrders.reserve(state->mOrders.size() + items.size());

            for (const auto &item : items)
            {
                const auto itemObject = item.toObject();
                const auto location = itemObject.value("location").toObject();
                const auto range = itemObject.value("range").toString();

                auto issued = QDateTime::fromString(itemObject.value("issued").toString(), Qt::ISODate);
                issued.setTimeSpec(Qt::UTC);

                ExternalOrder order;

                order.setId(itemObject.value("id_str").toString().toULongLong());
                order.setType((itemObject.value("buy").toBool()) ? (ExternalOrder::Type::Buy) : (ExternalOrder::Type::Sell));
                order.setTypeId(typeId);
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
                order.setPrice(itemObject.value("price").toDouble());
                order.setVolumeEntered(itemObject.value("volumeEntered").toInt());
                order.setVolumeRemaining(itemObject.value("volume").toInt());
                order.setMinVolume(itemObject.value("minVolume").toInt());
                order.setIssued(issued);
                order.setDuration(itemObject.value("duration").toInt());

                state->mOrders.emplace_back(std::move(order));
            }

            if (state->mState == OrderState::State::GotResponse)
                callback(std::move(state->mOrders), QString{});
            else
                state->mState = OrderState::State::GotResponse;
        };

        mInterface.fetchBuyMarketOrders(regionId, typeId, ifaceCallback);
        mInterface.fetchSellMarketOrders(regionId, typeId, ifaceCallback);
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

    void CRESTManager::handleNewPreferences()
    {
        QSettings settings;

        const auto rate = settings.value(CRESTSettings::rateLimitKey, CRESTSettings::rateLimitDefault).toFloat();
        CRESTInterface::setRateLimit(rate);
    }

    void CRESTManager::fetchEndpoints()
    {
        qDebug() << "Fetching CREST endpoints...";

        QNetworkRequest request{CRESTInterface::crestUrl};
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));
        request.setRawHeader("Accept", "application/vnd.ccp.eve.Api-v3+json");

        auto reply = mNetworkManager.get(request);
        Q_ASSERT(reply != nullptr);

        new ReplyTimeout{*reply};

        connect(reply, &QNetworkReply::finished, this, [=] {
            reply->deleteLater();

            const auto error = reply->error();
            qDebug() << "Got CREST endpoints: " << error;

            if (error != QNetworkReply::NoError)
            {
                QMessageBox::warning(nullptr, tr("CREST error"), tr("Error fetching CREST endpoints!"));
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

    QString CRESTManager::getMissingEnpointsError()
    {
        return tr("CREST endpoint map is empty. Please wait a while.");
    }
}
