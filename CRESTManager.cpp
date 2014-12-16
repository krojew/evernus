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
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "EveDataProvider.h"
#include "ExternalOrder.h"

#include "CRESTManager.h"

namespace Evernus
{
    CRESTManager::CRESTManager(QByteArray clientId, QByteArray clientSecret, const EveDataProvider &dataProvider)
        : mDataProvider{dataProvider}
        , mInterface{std::move(clientId), std::move(clientSecret)}
    {
    }

    void CRESTManager::fetchMarketOrders(uint regionId,
                                         EveType::IdType typeId,
                                         const Callback<std::vector<ExternalOrder>> &callback) const
    {
        if (mInterface.hasClientCredentials())
        {
            mInterface.fetchBuyMarketOrders(regionId, typeId, [=](QJsonDocument &&buyData, const QString &error) {
                if (!error.isEmpty())
                {
                    callback(std::vector<ExternalOrder>{}, error);
                    return;
                }

                mInterface.fetchSellMarketOrders(regionId, typeId, [=](QJsonDocument &&sellData, const QString &error) {
                    if (!error.isEmpty())
                    {
                        callback(std::vector<ExternalOrder>{}, error);
                        return;
                    }

                    std::vector<ExternalOrder> orders;
                    auto appendOrders = [=, &orders](const QJsonObject &object) {
                        QRegularExpression idRe{"/(\\d+)/$"};

                        const auto items = object.value("items").toArray();
                        for (const auto &item : items)
                        {
                            const auto itemObject = item.toObject();
                            const auto localtion = itemObject.value("location").toObject();
                            const auto range = itemObject.value("range").toString();

                            auto issued = QDateTime::fromString(itemObject.value("issued").toString(), Qt::ISODate);
                            issued.setTimeSpec(Qt::UTC);

                            ExternalOrder order;

                            // TODO: replace when ids become available
                            order.setId(idRe.match(itemObject.value("href").toString()).captured(1).toULongLong());
                            order.setType((itemObject.value("buy").toBool()) ? (ExternalOrder::Type::Buy) : (ExternalOrder::Type::Sell));
                            order.setTypeId(typeId);
                            order.setStationId(idRe.match(localtion.value("href").toString()).captured(1).toUInt());
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
                            // TODO: replace when available
                            order.setVolumeEntered(itemObject.value("volume").toInt());
                            order.setVolumeRemaining(itemObject.value("volume").toInt());
                            order.setMinVolume(itemObject.value("minVolume").toInt());
                            order.setIssued(issued);
                            order.setDuration(itemObject.value("duration").toInt());

                            orders.emplace_back(std::move(order));
                        }
                    };

                    appendOrders(buyData.object());
                    appendOrders(sellData.object());

                    callback(std::move(orders), QString{});
                });
            });
        }
        else
        {
            callback(std::vector<ExternalOrder>{},
                     "Evernus has been compiled without CREST support. "
                     "You can manually specify CREST client id and secret via command line options: --crest-id and --crest-secret"
            );
        }
    }
}
