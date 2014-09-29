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

#include <QNetworkAccessManager>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDateTime>
#include <QDebug>

#include "ExternalOrderRepository.h"

#include "UploaderThread.h"

namespace Evernus
{
    UploaderThread::UploaderThread(const ExternalOrderRepository &orderRepo, QObject *parent)
        : QThread{parent}
        , mOrderRepo{orderRepo}
    {
        mWaitTimer.setInterval(60 * 1000);
        mWaitTimer.start();
        connect(&mWaitTimer, &QTimer::timeout, this, &UploaderThread::scheduleUpload);
    }

    void UploaderThread::setEnabled(bool flag)
    {
        qDebug() << "Enabling upload:" << flag;
        mEnabled = flag;
    }

    void UploaderThread::handleChangedData()
    {
        mDataChanged = true;
    }

    void UploaderThread::scheduleUpload()
    {
        if (mEnabled)
            mDoUpload = true;
    }

    void UploaderThread::finishUpload()
    {
        auto reply = static_cast<QNetworkReply *>(sender());
        reply->deleteLater();

        const auto error = reply->error();

        qDebug() << "Upload status from" << reply->url() << ":" << error;
        qDebug() << "Data:" << reply->readAll();
    }

    void UploaderThread::run()
    {
        QNetworkAccessManager accessManager;
        QEventLoop eventLoop;

        while (!isInterruptionRequested())
        {
            eventLoop.processEvents();

            if (!mDoUpload || !mDataChanged)
            {
                sleep(1);
                continue;
            }

            qDebug() << "Uploading market data...";

            mDoUpload = false;

            std::vector<const Endpoint *> endpoints;
            for (const auto &endpoint : mEndpoints)
            {
                if (endpoint.mEnabled)
                    endpoints.emplace_back(&endpoint);
            }

            if (endpoints.empty())
            {
                qDebug() << "No enabled endpoints found.";
                continue;
            }

            mDataChanged = false;

            QJsonObject data;
            data.insert("resultType", "orders");
            data.insert("version", "0.1");

            QJsonArray uploadKeys;
            for (const auto endpoint : endpoints)
            {
                QJsonObject uploadKey;
                uploadKey.insert("name", endpoint->mName);
                uploadKey.insert("key", endpoint->mKey);

                uploadKeys << uploadKey;
            }

            data.insert("uploadKeys", uploadKeys);

            QJsonObject generator;
            generator.insert("name", QCoreApplication::applicationName());
            generator.insert("version", QCoreApplication::applicationVersion());

            data.insert("generator", generator);
            data.insert("currentTime", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

            QJsonArray columns;
            columns << "price";
            columns << "volRemaining";
            columns << "range";
            columns << "orderID";
            columns << "volEntered";
            columns << "minVolume";
            columns << "bid";
            columns << "issueDate";
            columns << "duration";
            columns << "stationID";
            columns << "solarSystemID";

            data.insert("columns", columns);

            std::unordered_map<EveType::IdType, QJsonObject> typeSets;
            std::unordered_map<EveType::IdType, QDateTime> typeDates;

            auto query = mOrderRepo.exec(QString{"SELECT * FROM %1"}.arg(mOrderRepo.getTableName()));
            while (query.next())
            {
                auto updateTime = query.value("update_time").toDateTime();
                updateTime.setTimeSpec(Qt::UTC);

                const auto typeId = query.value("type_id").toInt();

                auto it = typeSets.find(typeId);
                if (it == std::end(typeSets))
                {
                    QJsonObject rowset;
                    rowset.insert("generatedAt", updateTime.toString(Qt::ISODate));
                    rowset.insert("regionID", QJsonValue{});
                    rowset.insert("typeID", typeId);
                    rowset.insert("rows", QJsonArray{});

                    it = typeSets.emplace(typeId, std::move(rowset)).first;
                }
                else
                {
                    auto dateIt = typeDates.find(typeId);
                    if (dateIt != std::end(typeDates))
                    {
                        if (dateIt->second < updateTime)
                        {
                            it->second.insert("rows", QJsonArray{});
                            dateIt->second = updateTime;
                        }
                        else if (dateIt->second > updateTime)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        typeDates.emplace(typeId, updateTime);
                    }
                }

                auto rows = it->second.find("rows");
                auto array = rows.value().toArray();

                auto issued = query.value("issued").toDateTime();
                issued.setTimeSpec(Qt::UTC);

                QJsonArray row;
                row << query.value("value").toDouble();
                row << query.value("volume_remaining").toInt();
                row << query.value("range").toInt();
                row << query.value(mOrderRepo.getIdColumn()).toDouble();
                row << query.value("volume_entered").toInt();
                row << query.value("min_volume").toInt();
                row << (static_cast<ExternalOrder::Type>(query.value("type").toInt()) == ExternalOrder::Type::Buy);
                row << issued.toString(Qt::ISODate);
                row << query.value("duration").toInt();
                row << query.value("location_id").toInt();
                row << query.value("solar_system_id").toInt();

                array << row;

                rows.value() = array;
            }

            QJsonArray rowsets;
            for (auto &rowset : typeSets)
                rowsets << std::move(rowset.second);

            if (rowsets.isEmpty())
            {
                qDebug() << "No order data to upload.";
                continue;
            }

            data.insert("rowsets", rowsets);

            QJsonDocument doc{data};
            const auto json = doc.toJson(QJsonDocument::Compact);

            for (const auto endpoint : endpoints)
            {
                qDebug() << "Uploading to" << endpoint->mUrl;

                QNetworkReply *reply = nullptr;

                switch (endpoint->mMethod) {
                case UploadMethod::Get:
                    {
                        QUrlQuery query;
                        query.addQueryItem("data", json);

                        QUrl url = endpoint->mUrl;
                        url.setQuery(query);

                        reply = accessManager.get(QNetworkRequest{url});
                    }
                    break;
                case UploadMethod::Post:
                    {
                        QUrlQuery query;
                        query.addQueryItem("data", json);

                        QNetworkRequest request{endpoint->mUrl};
                        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

                        reply = accessManager.post(request, query.toString(QUrl::FullyEncoded).toUtf8());
                    }
                    break;
                case UploadMethod::PostEntity:
                    {
                        QNetworkRequest request{endpoint->mUrl};
                        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

                        reply = accessManager.post(request, json);
                    }
                    break;
                case UploadMethod::Put:
                    reply = accessManager.put(QNetworkRequest{endpoint->mUrl}, json);
                }

                Q_ASSERT(reply != nullptr);
                connect(reply, &QNetworkReply::finished, this, &UploaderThread::finishUpload);
            }
        }
    }
}
