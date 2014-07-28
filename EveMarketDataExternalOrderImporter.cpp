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
#include <QAbstractMessageHandler>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QXmlQuery>

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
        
        QUrlQuery query;
        query.addQueryItem("buysell", "a");
        query.addQueryItem("char_name", QCoreApplication::applicationName());

        QStringList typeIds, stationIds;
        for (const auto &pair : target)
        {
            typeIds << QString::number(pair.first);
            stationIds << QString::number(pair.second);
        }

        query.addQueryItem("type_ids", typeIds.join(','));
        query.addQueryItem("station_ids", stationIds.join(','));

        QUrl url{"http://api.eve-marketdata.com/api/item_orders2.xml"};
        url.setQuery(query);

        auto reply = mNetworkManager.get(QNetworkRequest{url});
        connect(reply, &QNetworkReply::finished, this, &EveMarketDataExternalOrderImporter::processReply);
    }

    void EveMarketDataExternalOrderImporter::processReply() const
    {
        auto reply = static_cast<QNetworkReply *>(sender());
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            emit error(reply->errorString());
            return;
        }

        QString errorMsg;
        EveMarketDataExternalOrderImporterXmlMessageHandler handler{errorMsg};

        QXmlQuery query;
        query.setMessageHandler(&handler);
        query.setFocus(reply->readAll());
        query.setQuery("//rowset[@name='orders']/row");

        EveMarketDataExternalOrderImporterXmlReceiver recevier{query.namePool()};
        query.evaluateTo(&recevier);

        if (errorMsg.isEmpty())
            emit externalOrdersChanged(std::move(recevier).getResult());
        else
            emit error(errorMsg);
    }
}
