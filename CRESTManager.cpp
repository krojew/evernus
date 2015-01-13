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
#include <QWebFrame>
#include <QSettings>
#include <QDebug>

#include "PersistentCookieJar.h"
#include "EveDataProvider.h"
#include "ExternalOrder.h"
#include "CRESTSettings.h"

#include "CRESTManager.h"

namespace Evernus
{
#ifdef EVERNUS_CREST_SISI
    const QString CRESTManager::loginUrl = "https://sisilogin.testeveonline.com";
#else
    const QString CRESTManager::loginUrl = "https://login-tq.eveonline.com";
#endif

    const QString CRESTManager::redirectUrl = "evernus.com";

    CRESTManager
    ::CRESTManager(QByteArray clientId, QByteArray clientSecret, const EveDataProvider &dataProvider, QObject *parent)
        : QObject{parent}
        , mDataProvider{dataProvider}
        , mClientId{std::move(clientId)}
        , mClientSecret{std::move(clientSecret)}
        , mCrypt{CRESTSettings::cryptKey}
    {
        QSettings settings;
        mRefreshToken = mCrypt.decryptToString(settings.value(CRESTSettings::refreshTokenKey).toByteArray());

        connect(&mInterface, &CRESTInterface::tokenRequested, this, &CRESTManager::fetchToken);
        connect(this, &CRESTManager::tokenError, &mInterface, &CRESTInterface::handleTokenError);
        connect(this, &CRESTManager::acquiredToken, &mInterface, &CRESTInterface::updateTokenAndContinue);
    }

    bool CRESTManager::eventFilter(QObject *watched, QEvent *event)
    {
        Q_ASSERT(event != nullptr);

        if (watched == mAuthView.get() && event->type() == QEvent::Close)
        {
            qDebug() << "Auth window closed.";
            emit tokenError(tr("CREST authorization failed."));
        }

        return QObject::eventFilter(watched, event);
    }

    void CRESTManager::fetchMarketOrders(uint regionId,
                                         EveType::IdType typeId,
                                         const Callback<std::vector<ExternalOrder>> &callback) const
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
                    const auto items = object.value("items").toArray();
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

                        orders.emplace_back(std::move(order));
                    }
                };

                appendOrders(buyData.object());
                appendOrders(sellData.object());

                callback(std::move(orders), QString{});
            });
        });
    }

    void CRESTManager::fetchMarketHistory(uint regionId,
                                          EveType::IdType typeId,
                                          const Callback<std::map<QDate, MarketHistoryEntry>> &callback) const
    {
        mInterface.fetchMarketHistory(regionId, typeId, [=](QJsonDocument &&data, const QString &error) {
            if (!error.isEmpty())
            {
                callback(std::map<QDate, MarketHistoryEntry>{}, error);
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

    bool CRESTManager::hasClientCredentials() const
    {
        return !mClientId.isEmpty() && !mClientSecret.isEmpty();
    }

    void CRESTManager::fetchToken()
    {
        qDebug() << "Refreshing access token...";

        if (mRefreshToken.isEmpty())
        {
            qDebug() << "No refresh token - requesting access.";

            QUrl url{loginUrl + "/oauth/authorize"};

            QUrlQuery query;
            query.addQueryItem("response_type", "code");
            query.addQueryItem("redirect_uri", "http://" + redirectUrl);
            query.addQueryItem("client_id", mClientId);
            query.addQueryItem("scope", "publicData");

            url.setQuery(query);

            mAuthView = std::make_unique<QWebView>();

            auto nam = mAuthView->page()->networkAccessManager();
            nam->setCookieJar(new PersistentCookieJar{CRESTSettings::cookiesKey});
            connect(nam, &QNetworkAccessManager::sslErrors, this, &CRESTManager::handleSslErrors);

            mAuthView->setWindowModality(Qt::ApplicationModal);
            mAuthView->setWindowTitle(tr("CREST Authentication"));
            mAuthView->installEventFilter(this);
            mAuthView->adjustSize();
            mAuthView->move(QApplication::desktop()->screenGeometry(QApplication::activeWindow()).center() -
                            mAuthView->rect().center());
            mAuthView->setUrl(url);
            mAuthView->show();

            connect(mAuthView->page()->mainFrame(), &QWebFrame::urlChanged, [=](const QUrl &url) {
                if (url.host() == redirectUrl)
                {
                    mAuthView->removeEventFilter(this);
                    mAuthView->close();

                    qDebug() << "Requesting access token...";

                    QUrlQuery query{url};
                    QByteArray data = "grant_type=authorization_code&code=";
                    data.append(query.queryItemValue("code"));

                    QNetworkRequest request{loginUrl + "/oauth/token"};
                    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
                    request.setRawHeader(
                        "Authorization", (mClientId + ":" + mClientSecret).toBase64());

                    auto reply = mNetworkManager.post(request, data);
                    connect(reply, &QNetworkReply::finished, this, [=] {
                        reply->deleteLater();

                        if (reply->error() != QNetworkReply::NoError)
                        {
                            qDebug() << "Error requesting access token:" << reply->errorString();
                            emit tokenError(reply->errorString());
                            return;
                        }

                        const auto doc = QJsonDocument::fromJson(reply->readAll());
                        const auto object = doc.object();

                        mRefreshToken = object.value("refresh_token").toString();
                        if (mRefreshToken.isEmpty())
                        {
                            qDebug() << "Empty refresh token!";
                            emit tokenError(tr("Empty refresh token!"));
                            return;
                        }

                        QSettings settings;
                        settings.setValue(CRESTSettings::refreshTokenKey, mCrypt.encryptToByteArray(mRefreshToken));

                        emit acquiredToken(object.value("access_token").toString(),
                                           QDateTime::currentDateTime().addSecs(object.value("expires_in").toInt() - 10));
                    });
                }
            });
        }
        else
        {
            qDebug() << "Refreshing token...";

            QByteArray data = "grant_type=refresh_token&refresh_token=";
            data.append(mRefreshToken);

            QNetworkRequest request{loginUrl + "/oauth/token"};
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            request.setRawHeader(
                "Authorization", (mClientId + ":" + mClientSecret).toBase64());

            auto reply = mNetworkManager.post(request, data);
            connect(reply, &QNetworkReply::finished, this, [=] {
                reply->deleteLater();

                const auto doc = QJsonDocument::fromJson(reply->readAll());
                const auto object = doc.object();

                if (reply->error() != QNetworkReply::NoError)
                {
                    qDebug() << "Error refreshing token:" << reply->errorString();

                    if (object.value("error") == "invalid_token")
                    {
                        mRefreshToken.clear();
                        fetchToken();
                    }
                    else
                    {
                        emit tokenError(reply->errorString());
                    }

                    return;
                }

                const auto accessToken = object.value("access_token").toString();
                if (accessToken.isEmpty())
                {
                    qDebug() << "Empty access token!";
                    emit tokenError(tr("Empty access token!"));
                    return;
                }

                emit acquiredToken(accessToken,
                                   QDateTime::currentDateTime().addSecs(doc.object().value("expires_in").toInt() - 10));
            });
        }
    }

    void CRESTManager::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
    {
        QStringList errorStrings;
        for (const auto &error : errors)
            errorStrings << error.errorString();

        const auto ret = QMessageBox::question(mAuthView.get(), tr("CREST error"), tr(
            "EVE login page certificate contains errors:\n%1\nAre you sure you wish to proceed (doing so can compromise your account security)?").arg(errorStrings.join("\n")));
        if (ret == QMessageBox::Yes)
            reply->ignoreSslErrors();
    }
}
