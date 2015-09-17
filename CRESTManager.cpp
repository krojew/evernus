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
#include <QWebPage>
#include <QDebug>

#include "PersistentCookieJar.h"
#include "EveDataProvider.h"
#include "ExternalOrder.h"
#include "CRESTSettings.h"
#include "ReplyTimeout.h"

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

#ifdef EVERNUS_CREST_SISI
    const QString CRESTManager::loginUrl = "https://sisilogin.testeveonline.com";
#else
    const QString CRESTManager::loginUrl = "https://login-tq.eveonline.com";
#endif

    const QString CRESTManager::redirectDomain = "evernus.com";

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

        selectNextInterface().fetchBuyMarketOrders(regionId, typeId, ifaceCallback);
        selectNextInterface().fetchSellMarketOrders(regionId, typeId, ifaceCallback);
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

#ifdef Q_OS_WIN
        selectNextInterface().fetchMarketHistory(regionId, typeId, [=](QJsonDocument &&data, const QString &error) {
#else
        selectNextInterface().fetchMarketHistory(regionId, typeId, [=, callback = callback](QJsonDocument &&data, const QString &error) {
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
                query.addQueryItem("scope", "publicData");

                url.setQuery(query);

                mAuthView = std::make_unique<CRESTAuthWidget>(url);

                auto nam = mAuthView->page()->networkAccessManager();
                nam->setCookieJar(new PersistentCookieJar{CRESTSettings::cookiesKey});
                connect(nam, &QNetworkAccessManager::sslErrors, this, &CRESTManager::handleSslErrors);

                mAuthView->setWindowModality(Qt::ApplicationModal);
                mAuthView->setWindowTitle(tr("CREST Authentication"));
                mAuthView->installEventFilter(this);
                mAuthView->adjustSize();
                mAuthView->move(QApplication::desktop()->screenGeometry(QApplication::activeWindow()).center() -
                                mAuthView->rect().center());
                mAuthView->show();

                connect(mAuthView.get(), &CRESTAuthWidget::acquiredCode, this, &CRESTManager::processAuthorizationCode);
                connect(mAuthView->page()->mainFrame(), &QWebFrame::urlChanged, [=](const QUrl &url) {
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
                            qDebug() << "Error refreshing token:" << reply->errorString();

                            const auto error = object.value("error").toString();

                            qDebug() << "Returned error:" << error;

                            if (error == "invalid_token" || error == "invalid_client")
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
                            qDebug() << "Empty access token!";
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
        for (auto iface : mInterfaces)
            iface->deleteLater();

        mInterfaces.clear();
        mCurrentInterface = 0;

        QSettings settings;

        // IO bound
        const auto maxInterfaces = settings.value(CRESTSettings::maxThreadsKey, CRESTSettings::maxThreadsDefault).toUInt();
        mInterfaces.reserve(maxInterfaces);

        for (auto i = 0u; i < maxInterfaces; ++i)
        {
            mInterfaces.emplace_back(new CRESTInterface{this});
            mInterfaces.back()->setEndpoints(mEndpoints);

            connect(mInterfaces.back(), &CRESTInterface::tokenRequested, this, &CRESTManager::fetchToken, Qt::QueuedConnection);
            connect(this, &CRESTManager::acquiredToken, mInterfaces.back(), &CRESTInterface::updateTokenAndContinue);
            connect(this, &CRESTManager::tokenError, mInterfaces.back(), &CRESTInterface::handleTokenError);
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

            for (const auto interface : mInterfaces)
                interface->setEndpoints(mEndpoints);
        });
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

    const CRESTInterface &CRESTManager::selectNextInterface() const
    {
        const auto &interface = *mInterfaces[mCurrentInterface];
        mCurrentInterface = (mCurrentInterface + 1) % mInterfaces.size();

        return interface;
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
