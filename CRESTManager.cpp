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
#include <QWebEnginePage>
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
#ifdef EVERNUS_CREST_SISI
    const QString CRESTManager::loginUrl = "https://sisilogin.testeveonline.com";
#else
    const QString CRESTManager::loginUrl = "https://login-tq.eveonline.com";
#endif

    const QString CRESTManager::redirectDomain = "evernus.com";

    QString CRESTManager::mRefreshToken;
    bool CRESTManager::mFetchingToken = false;

    CRESTManager::CRESTManager(QByteArray clientId,
                               QByteArray clientSecret,
                               const EveDataProvider &dataProvider,
                               QObject *parent)
        : QObject{parent}
        , mDataProvider{dataProvider}
        , mClientId{std::move(clientId)}
        , mClientSecret{std::move(clientSecret)}
        , mCrypt{CRESTSettings::cryptKey}
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

        QSettings settings;
        mRefreshToken = mCrypt.decryptToString(settings.value(CRESTSettings::refreshTokenKey).toByteArray());

        connect(&mInterface, &CRESTInterface::tokenRequested, this, &CRESTManager::fetchToken, Qt::QueuedConnection);
        connect(this, &CRESTManager::acquiredToken, &mInterface, &CRESTInterface::updateTokenAndContinue);
        connect(this, &CRESTManager::tokenError, &mInterface, &CRESTInterface::handleTokenError);
        connect(this, &CRESTManager::tokenError, this, &CRESTManager::error);
    }

    bool CRESTManager::eventFilter(QObject *watched, QEvent *event)
    {
        Q_ASSERT(event != nullptr);

        if (watched == mAuthView.get() && event->type() == QEvent::Close)
        {
            mFetchingToken = false;

            qDebug() << "Auth window closed.";
            emit tokenError(tr("CREST authorization failed."));
        }

        return QObject::eventFilter(watched, event);
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

    void CRESTManager::fetchMarketOrders(uint regionId, const Callback<std::vector<ExternalOrder>> &callback) const
    {
        if (!hasEndpoints())
        {
            callback(std::vector<ExternalOrder>{}, getMissingEnpointsError());
            return;
        }

        auto orders = std::make_shared<std::vector<ExternalOrder>>();
        mInterface.fetchMarketOrders(regionId, [=, orders = std::move(orders)](auto &&data, auto atEnd, const auto &error) {
            if (!error.isEmpty())
            {
                callback(std::vector<ExternalOrder>{}, error);
                return;
            }

            const auto items = data.object().value("items").toArray();
            orders->reserve(orders->size() + items.size());

            for (const auto &item : items)
                orders->emplace_back(getOrderFromJson(item.toObject(), regionId));

            if (atEnd)
                callback(std::move(*orders), QString{});
        });
    }

    void CRESTManager::openMarketDetails(EveType::IdType typeId, Character::IdType charId) const
    {
        mInterface.openMarketDetails(typeId, charId, [=](const auto &errorText) {
            emit error(errorText);
        });
    }

    bool CRESTManager::hasClientCredentials() const
    {
        return !mClientId.isEmpty() && !mClientSecret.isEmpty();
    }

    void CRESTManager::fetchToken()
    {
        if (mFetchingToken)
            return;

        mFetchingToken = true;

        try
        {
            qDebug() << "Refreshing access token...";

            if (mRefreshToken.isEmpty())
            {
                qDebug() << "No refresh token - requesting access.";

                QUrl url{loginUrl + "/oauth/authorize"};

                QUrlQuery query;
                query.addQueryItem("response_type", "code");
                query.addQueryItem("redirect_uri", "http://" + redirectDomain + "/crest-authentication/");
                query.addQueryItem("client_id", mClientId);
                query.addQueryItem("scope", "characterNavigationWrite remoteClientUI");

                url.setQuery(query);

                mAuthView = std::make_unique<CRESTAuthWidget>(url);

                mAuthView->setWindowModality(Qt::ApplicationModal);
                mAuthView->setWindowTitle(tr("CREST Authentication"));
                mAuthView->installEventFilter(this);
                mAuthView->adjustSize();
                mAuthView->move(QApplication::desktop()->screenGeometry(QApplication::activeWindow()).center() -
                                mAuthView->rect().center());
                mAuthView->show();

                connect(mAuthView.get(), &CRESTAuthWidget::acquiredCode, this, &CRESTManager::processAuthorizationCode);
                connect(mAuthView->page(), &QWebEnginePage::urlChanged, [=](const QUrl &url) {
                    try
                    {
                        if (url.host() == redirectDomain)
                        {
                            QUrlQuery query{url};
                            processAuthorizationCode(query.queryItemValue("code").toLatin1());
                        }
                    }
                    catch (...)
                    {
                        mFetchingToken = false;
                        throw;
                    }
                });
            }
            else
            {
                qDebug() << "Refreshing token...";

                QByteArray data = "grant_type=refresh_token&refresh_token=";
                data.append(mRefreshToken);

                auto reply = mNetworkManager.post(getAuthRequest(), data);
                connect(reply, &QNetworkReply::finished, this, [=] {
                    try
                    {
                        reply->deleteLater();

                        const auto doc = QJsonDocument::fromJson(reply->readAll());
                        const auto object = doc.object();

                        if (reply->error() != QNetworkReply::NoError)
                        {
                            qWarning() << "Error refreshing token:" << reply->errorString();

                            const auto error = object.value("error").toString();

                            qWarning() << "Returned error:" << error;

                            if (error == "invalid_token" || error == "invalid_client" || error == "invalid_grant")
                            {
                                mRefreshToken.clear();
                                mFetchingToken = false;
                                fetchToken();
                            }
                            else
                            {
                                const auto desc = object.value("error_description").toString();
                                emit tokenError((desc.isEmpty()) ? (reply->errorString()) : (desc));
                            }

                            return;
                        }

                        const auto accessToken = object.value("access_token").toString();
                        if (accessToken.isEmpty())
                        {
                            qWarning() << "Empty access token!";
                            emit tokenError(tr("Empty access token!"));
                            return;
                        }

                        emit acquiredToken(accessToken,
                                           QDateTime::currentDateTime().addSecs(doc.object().value("expires_in").toInt() - 10));

                        mFetchingToken = false;
                    }
                    catch (...)
                    {
                        mFetchingToken = false;
                        throw;
                    }
                });
            }
        }
        catch (...)
        {
            mFetchingToken = false;
            throw;
        }
    }

    void CRESTManager::handleNewPreferences()
    {
        QSettings settings;

        const auto rate = settings.value(CRESTSettings::rateLimitKey, CRESTSettings::rateLimitDefault).toFloat();
        CRESTInterface::setRateLimit(rate);
    }

    void CRESTManager::processAuthorizationCode(const QByteArray &code)
    {
        try
        {
            mAuthView->removeEventFilter(this);
            mAuthView->close();

            qDebug() << "Requesting access token...";

            QByteArray data = "grant_type=authorization_code&code=" + code;

            auto reply = mNetworkManager.post(getAuthRequest(), data);
            connect(reply, &QNetworkReply::finished, this, [=] {
                try
                {
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

                    mFetchingToken = false;
                }
                catch (...)
                {
                    mFetchingToken = false;
                    throw;
                }
            });
        }
        catch (...)
        {
            mFetchingToken = false;
            throw;
        }
    }

    QNetworkRequest CRESTManager::getAuthRequest() const
    {
        QNetworkRequest request{loginUrl + "/oauth/token"};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        request.setRawHeader(
            "Authorization", "Basic " + (mClientId + ":" + mClientSecret).toBase64());

        return request;
     }

    void CRESTManager::fetchEndpoints()
    {
        qDebug() << "Fetching CREST endpoints:" << CRESTInterface::crestUrl;

        QNetworkRequest request{CRESTInterface::crestUrl};
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          QString{"%1 %2"}.arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));
        request.setRawHeader("Accept", "application/vnd.ccp.eve.Api-v5+json");

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
        const auto range = object.value("range").toString();

        auto issued = QDateTime::fromString(object.value("issued").toString(), Qt::ISODate);
        issued.setTimeSpec(Qt::UTC);

        ExternalOrder order;

        order.setId(object.value("id").toDouble()); // https://bugreports.qt.io/browse/QTBUG-28560
        order.setType((object.value("buy").toBool()) ? (ExternalOrder::Type::Buy) : (ExternalOrder::Type::Sell));

        const auto type = object.value("type");
        order.setTypeId((type.isObject()) ? (type.toObject().value("id").toInt()) : (type.toInt()));

        const auto location = object.value("location");
        if (location.isUndefined())
            order.setStationId(object.value("stationID").toInt());
        else
            order.setStationId(location.toObject().value("id_str").toString().toUInt());

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
