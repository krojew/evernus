#include <QAbstractMessageHandler>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QXmlQuery>

#include "EveMarketDataItemPriceImporterXmlReceiver.h"

#include "EveMarketDataItemPriceImporter.h"

namespace Evernus
{
    namespace
    {
        class EveMarketDataItemPriceImporterXmlMessageHandler
            : public QAbstractMessageHandler
        {
        public:
            explicit EveMarketDataItemPriceImporterXmlMessageHandler(QString &error)
                : QAbstractMessageHandler{}
                , mError{error}
            {
            }

            virtual ~EveMarketDataItemPriceImporterXmlMessageHandler() = default;

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

    void EveMarketDataItemPriceImporter::fetchItemPrices(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit itemPricesChanged(std::vector<ItemPrice>{});
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

        QUrl url{"http://api.eve-marketdata.com/api/item_prices2.xml"};
        url.setQuery(query);

        auto reply = mNetworkManager.get(QNetworkRequest{url});
        connect(reply, &QNetworkReply::finished, this, [target, this]{
            processReply(target);
        });
    }

    void EveMarketDataItemPriceImporter::processReply(const TypeLocationPairs &target) const
    {
        auto reply = static_cast<QNetworkReply *>(sender());
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            emit error(reply->errorString());
            return;
        }

        QString errorMsg;
        EveMarketDataItemPriceImporterXmlMessageHandler handler{errorMsg};

        QXmlQuery query;
        query.setMessageHandler(&handler);
        query.setFocus(reply->readAll());
        query.setQuery("//rowset[@name='item_prices']/row");

        EveMarketDataItemPriceImporterXmlReceiver recevier{target, query.namePool()};
        query.evaluateTo(&recevier);

        if (errorMsg.isEmpty())
            emit itemPricesChanged(std::move(recevier).getResult());
        else
            emit error(errorMsg);
    }
}
