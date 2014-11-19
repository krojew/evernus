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
#include <future>

#include <QAbstractMessageHandler>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMultiHash>
#include <QUrlQuery>
#include <QXmlQuery>
#include <QDebug>

#include "EveMarketDataExternalOrderImporterXmlReceiver.h"

#include "EveMarketDataExternalOrderImporter.h"

namespace Evernus
{
    namespace
    {
        class EveMarketDataExternalOrderImporterXmlMessageHandler
            : public QAbstractMessageHandler
        {
        public:
            explicit EveMarketDataExternalOrderImporterXmlMessageHandler(QString &error)
                : QAbstractMessageHandler{}
                , mError{error}
            {
            }

            virtual ~EveMarketDataExternalOrderImporterXmlMessageHandler() = default;

        protected:
            virtual void handleMessage(QtMsgType type, const QString &description, const QUrl &identifier, const QSourceLocation &sourceLocation) override
            {
                if (type == QtFatalMsg && mError.isEmpty())
                    mError = QString{"%1 (%2:%3)"}.arg(description).arg(sourceLocation.line()).arg(sourceLocation.column());
            }

        private:
            QString &mError;
        };
    }

    void EveMarketDataExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
            return;
        }

        QMultiHash<quint64, EveType::IdType> requests;
        for (const auto &pair : target)
            requests.insert(pair.second, pair.first);

        const auto requestSend = [this](const auto &typeIds, const auto &stationId) {
            QUrlQuery query;
            query.addQueryItem("buysell", "a");
            query.addQueryItem("char_name", QCoreApplication::applicationName());
            query.addQueryItem("type_ids", typeIds.join(','));
            query.addQueryItem("station_ids", stationId);

            QUrl url{"http://api.eve-marketdata.com/api/item_orders2.xml"};
            url.setQuery(query);

            auto reply = mNetworkManager.get(QNetworkRequest{url});
            connect(reply, &QNetworkReply::finished, this, &Evernus::EveMarketDataExternalOrderImporter::processReply);

            ++mRequestCount;
        };

        const auto locations = requests.uniqueKeys();
        for (const auto &location : locations)
        {
            qDebug() << "Sending request for location:" << location;

            const auto maxBatchSize = 100;
            QStringList typeIds;

            auto counter = 0;
            for (auto it = requests.find(location); it != std::end(requests) && it.key() == location; ++it)
            {
                typeIds << QString::number(it.value());

                ++counter;
                if (counter == maxBatchSize)
                {
                    requestSend(typeIds, QString::number(location));
                    typeIds.clear();

                    counter = 0;
                }
            }

            if (counter != 0)
                requestSend(typeIds, QString::number(location));
        }

        qDebug() << "Total requests:" << mRequestCount;
    }

    void EveMarketDataExternalOrderImporter::processReply() const
    {
        auto reply = static_cast<QNetworkReply *>(sender());
        reply->deleteLater();

        --mRequestCount;

        qDebug() << "Got reply," << mRequestCount << "remaining.";

        if (reply->error() != QNetworkReply::NoError)
        {
            if (mRequestCount == 0)
                mResult.clear();

            emit error(reply->errorString());
            return;
        }

        QString errorMsg;
        EveMarketDataExternalOrderImporterXmlMessageHandler handler{errorMsg};

        // QXmlQuery creates a message loop - BAD things will happen when launched in this thread
        auto task = std::async(std::launch::async, [reply, &handler]{
            QXmlQuery query;
            query.setMessageHandler(&handler);
            query.setFocus(reply->readAll());
            query.setQuery("//rowset[@name='orders']/row");

            EveMarketDataExternalOrderImporterXmlReceiver recevier{query.namePool()};
            query.evaluateTo(&recevier);

            return std::move(recevier).getResult();
        });
        auto &&result = task.get();

        if (errorMsg.isEmpty())
        {
            mResult.reserve(mResult.size() + result.size());
            mResult.insert(std::end(mResult),
                           std::make_move_iterator(std::begin(result)),
                           std::make_move_iterator(std::end(result)));

            if (mRequestCount == 0)
            {
                emit externalOrdersChanged(mResult);
                mResult.clear();
            }
        }
        else
        {
            if (mRequestCount == 0)
                mResult.clear();

            emit error(errorMsg);
        }
    }
}
