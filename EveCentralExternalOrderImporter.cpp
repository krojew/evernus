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

#include <boost/scope_exit.hpp>

#include "EveDataProvider.h"

#include "EveCentralExternalOrderImporter.h"

namespace Evernus
{
    EveCentralExternalOrderImporter::EveCentralExternalOrderImporter(const EveDataProvider &dataProvider, QObject *parent)
        : ExternalOrderImporter{parent}
        , mDataProvider{dataProvider}
    {
    }

    void EveCentralExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
            return;
        }

        mPreparingRequests = true;
        BOOST_SCOPE_EXIT(this_) {
            this_->mPreparingRequests = false;
        } BOOST_SCOPE_EXIT_END

        qDebug() << "Fetching" << target.size() << "orders from eve-central.";

        std::unordered_multimap<TypeLocationPair::first_type, uint> aggregated;
        for (const auto &pair : target)
        {
            const auto regionId = mDataProvider.getStationRegionId(pair.second);
            if (regionId != 0)
                aggregated.emplace(pair.first, regionId);
        }

        qDebug() << "Aggregated orders:" << aggregated.size();

        mCounter.resetBatchIfEmpty();

        for (auto it = std::begin(aggregated); it != std::end(aggregated);)
        {
            const auto end = aggregated.equal_range(it->first).second;
            const auto typeId = it->first;

            QUrlQuery query{"typeid=" + QString::number(typeId)};

            do {
                query.addQueryItem("regionlimit", QString::number(it->second));
            } while (++it != end);

            QUrl url{"http://api.eve-central.com/api/quicklook"};
            url.setQuery(query);

            qDebug() << "Fetching" << url;

            QNetworkRequest request{url};
            request.setHeader(QNetworkRequest::UserAgentHeader,
                              QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));

            mCounter.incCount();

            auto reply = mNetworkManager.get(request);
            connect(reply, &QNetworkReply::finished, this, [=] {
                reply->deleteLater();

                const auto replyError = reply->error();
                if (replyError != QNetworkReply::NoError)
                    mAggregatedErrors << reply->errorString();

                processResult(typeId, reply->readAll());
            });
        }

        if (mCounter.isEmpty())
        {
            if (mAggregatedErrors.isEmpty())
                emit externalOrdersChanged(mResult);
            else
                emit error(mAggregatedErrors.join("\n"));

            mResult.clear();
        }
    }

    void EveCentralExternalOrderImporter::processResult(ExternalOrder::TypeIdType typeId, const QByteArray &result) const
    {
        if (mCounter.advanceAndCheckBatch())
            emit statusChanged(tr("Waiting for %1 eve-central replies...").arg(mCounter.getCount()));

        qDebug() << "Got reply," << mCounter.getCount() << "remaining.";

        if (mAggregatedErrors.isEmpty())
        {
            QXmlStreamReader reader{result};
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

                        mResult.emplace_back(std::move(order));
                        order.setId(ExternalOrder::invalidId);
                    }
                }
            }

            if (reader.hasError())
                mAggregatedErrors << reader.errorString();
        }

        if (mCounter.isEmpty() && !mPreparingRequests)
        {
            if (mAggregatedErrors.isEmpty())
                emit externalOrdersChanged(mResult);
            else
                emit error(mAggregatedErrors.join("\n"));

            mResult.clear();
        }
    }
}
