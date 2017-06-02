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
#include <algorithm>
#include <atomic>

#include <QtConcurrent>

#include <QFutureWatcher>
#include <QDesktopWidget>
#include <QWebEnginePage>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QApplication>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QUrlQuery>
#include <QSettings>
#include <QDateTime>
#include <QDebug>

#include <boost/scope_exit.hpp>

#include "CharacterRepository.h"
#include "EveDataProvider.h"
#include "NetworkSettings.h"
#include "ExternalOrder.h"
#include "ReplyTimeout.h"
#include "SSOSettings.h"
#include "Defines.h"

#include "ESIManager.h"

namespace Evernus
{
#ifdef EVERNUS_ESI_SISI
    const QString ESIManager::loginUrl = "https://sisilogin.testeveonline.com";
#else
    const QString ESIManager::loginUrl = "https://login.eveonline.com";
#endif

    const QString ESIManager::redirectDomain = "evernus.com";
    const QString ESIManager::firstTimeCitadelOrderImportKey = "import/firstTimeCitadelOrderImport";

    std::unordered_map<Character::IdType, QString> ESIManager::mRefreshTokens;
    bool ESIManager::mFetchingToken = false;

    bool ESIManager::mFirstTimeCitadelOrderImport = true;

    ESIManager::ESIManager(QByteArray clientId,
                           QByteArray clientSecret,
                           const EveDataProvider &dataProvider,
                           const CharacterRepository &characterRepo,
                           QObject *parent)
        : QObject{parent}
        , mDataProvider{dataProvider}
        , mCharacterRepo{characterRepo}
        , mClientId{std::move(clientId)}
        , mClientSecret{std::move(clientSecret)}
        , mCrypt{SSOSettings::cryptKey}
    {
        QSettings settings;
        settings.beginGroup(SSOSettings::refreshTokenGroup);

        const auto keys = settings.childKeys();
        for (const auto &key : keys)
            mRefreshTokens[key.toULongLong()] = mCrypt.decryptToString(settings.value(key).toByteArray());

        settings.endGroup();

        mFirstTimeCitadelOrderImport = settings.value(firstTimeCitadelOrderImportKey, mFirstTimeCitadelOrderImport).toBool();

        createInterfaces();

        connect(this, &ESIManager::tokenError, this, &ESIManager::error);
    }

    bool ESIManager::eventFilter(QObject *watched, QEvent *event)
    {
        Q_ASSERT(event != nullptr);

        if (watched == mAuthView.get() && event->type() == QEvent::Close)
        {
            mFetchingToken = false;

            qDebug() << "Auth window closed.";
            emit tokenError(tr("SSO authorization failed."));
        }

        return QObject::eventFilter(watched, event);
    }

    void ESIManager::fetchMarketOrders(uint regionId,
                                       EveType::IdType typeId,
                                       const MarketOrderCallback &callback) const
    {
        qDebug() << "Started market order import at" << QDateTime::currentDateTime();

        auto ifaceCallback = [=](QJsonDocument &&data, const QString &error) {
            if (!error.isEmpty())
            {
                callback(ExternalOrderList{}, error);
                return;
            }

            const auto watcher = new QFutureWatcher<ExternalOrderList>{};
            connect(watcher, &QFutureWatcher<ExternalOrderList>::finished, this, [=] {
                watcher->deleteLater();

                const auto future = watcher->future();
                callback(future.result(), QString{});
            });

            watcher->setFuture(QtConcurrent::run([=, items = data.array()] {
                ExternalOrderList result;
                result.reserve(items.size());

                for (const auto &item : items)
                    result.emplace_back(getOrderFromJson(item.toObject(), regionId));

                return result;
            }));
        };

        selectNextInterface().fetchMarketOrders(regionId, typeId, ifaceCallback);
    }

    void ESIManager::fetchMarketHistory(uint regionId,
                                        EveType::IdType typeId,
                                        const Callback<HistoryMap> &callback) const
    {
        qDebug() << "Started history import at" << QDateTime::currentDateTime();

#if EVERNUS_CLANG_LAMBDA_CAPTURE_BUG
        selectNextInterface().fetchMarketHistory(regionId, typeId, [=, callback = callback](QJsonDocument &&data, const QString &error) {
#else
        selectNextInterface().fetchMarketHistory(regionId, typeId, [=](QJsonDocument &&data, const QString &error) {
#endif
            if (!error.isEmpty())
            {
                callback(HistoryMap{}, error);
                return;
            }

            const std::function<std::pair<QDate, MarketHistoryEntry> (const QJsonValue &)> parseItem = [](const auto &item) {
                const auto itemObject = item.toObject();
                auto date = QDate::fromString(itemObject.value("date").toString(), Qt::ISODate);

                MarketHistoryEntry entry;
                entry.mAvgPrice = itemObject.value("average").toDouble();
                entry.mHighPrice = itemObject.value("highest").toDouble();
                entry.mLowPrice = itemObject.value("lowest").toDouble();
                entry.mOrders = itemObject.value("order_count").toInt();
                entry.mVolume = itemObject.value("volume").toDouble();

                return std::make_pair(std::move(date), std::move(entry));
            };

            const auto insertItem = [](auto &history, auto &item) {
                history.insert(std::move(item));
            };

            callback(QtConcurrent::blockingMappedReduced<HistoryMap>(data.array(), parseItem, insertItem), QString{});
        });
    }

    void ESIManager::fetchMarketOrders(uint regionId, const MarketOrderCallback &callback) const
    {
        qDebug() << "Started market order import at" << QDateTime::currentDateTime();
        selectNextInterface().fetchMarketOrders(regionId, getMarketOrderCallback(regionId, callback));
    }

    void ESIManager::fetchCitadelMarketOrders(quint64 citadelId, uint regionId, Character::IdType charId, const MarketOrderCallback &callback) const
    {
        if (mFirstTimeCitadelOrderImport)
        {
            mFirstTimeCitadelOrderImport = false;

            QSettings settings;
            settings.setValue(firstTimeCitadelOrderImportKey, false);

            QMessageBox::information(nullptr, tr("Citadel order import"), tr(
                "Seems like you are importing citadel orders for the first time. CCP only allows importing orders from citadels you have access to. "
                "This means you need to authenticate yourself with Eve SSO, if you haven't done that already (please wait for the SSO window to open).\n\n"
                "Also, please note that due to large numbers of citadels in some regions, the import might take much longer. Remember you can toggle citadel import "
                "in the Preferences."
            ));
        }

        qDebug() << "Started citadel market order import at" << QDateTime::currentDateTime();
        selectNextInterface().fetchCitadelMarketOrders(citadelId, charId, getMarketOrderCallback(regionId, callback));
    }

    void ESIManager::openMarketDetails(EveType::IdType typeId, Character::IdType charId) const
    {
        selectNextInterface().openMarketDetails(typeId, charId, [=](const auto &errorText) {
            emit error(errorText);
        });
    }

    void ESIManager::setDestination(quint64 locationId, Character::IdType charId) const
    {
        selectNextInterface().setDestination(locationId, charId, [=](const auto &errorText) {
            emit error(errorText);
        });
    }

    bool ESIManager::hasClientCredentials() const
    {
        return !mClientId.isEmpty() && !mClientSecret.isEmpty();
    }

    void ESIManager::fetchToken(Character::IdType charId)
    {
        if (mFetchingToken)
            return;

        mFetchingToken = true;

        try
        {
            qDebug() << "Refreshing access token for" << charId;

            const auto it = mRefreshTokens.find(charId);
            if (it == std::end(mRefreshTokens))
            {
                qDebug() << "No refresh token - requesting access.";

                QUrl url{loginUrl + "/oauth/authorize"};

                QUrlQuery query;
                query.addQueryItem("response_type", "code");
                query.addQueryItem("redirect_uri", "http://" + redirectDomain + "/sso-authentication/");
                query.addQueryItem("client_id", mClientId);
                query.addQueryItem("scope", "esi-ui.open_window.v1 esi-ui.write_waypoint.v1 esi-markets.structure_markets.v1");

                url.setQuery(query);

                mAuthView = std::make_unique<SOOAuthWidget>(url);

                mAuthView->setWindowModality(Qt::ApplicationModal);
                mAuthView->setWindowTitle(tr("SSO Authentication for character: %1").arg(mCharacterRepo.getName(charId)));
                mAuthView->installEventFilter(this);
                mAuthView->adjustSize();
                mAuthView->move(QApplication::desktop()->screenGeometry(QApplication::activeWindow()).center() -
                                mAuthView->rect().center());
                mAuthView->show();

                connect(mAuthView.get(), &SOOAuthWidget::acquiredCode, this, [=](const auto &code) {
                    processAuthorizationCode(charId, code);
                });
                connect(mAuthView->page(), &QWebEnginePage::urlChanged, this, [=](const QUrl &url) {
                    try
                    {
                        if (url.host() == redirectDomain)
                        {
                            QUrlQuery query{url};
                            processAuthorizationCode(charId, query.queryItemValue("code").toLatin1());
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
                data.append(it->second);

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
                                mRefreshTokens.erase(charId);
                                mFetchingToken = false;
                                fetchToken(charId);
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

                        emit acquiredToken(charId, accessToken,
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

    void ESIManager::handleNewPreferences()
    {
        for (auto iface : mInterfaces)
            iface->deleteLater();

        mInterfaces.clear();

        createInterfaces();
    }

    void ESIManager::processAuthorizationCode(Character::IdType charId, const QByteArray &code)
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
                        mFetchingToken = false;

                        qDebug() << "Error requesting access token:" << reply->errorString();
                        emit tokenError(reply->errorString());
                        return;
                    }

                    const auto doc = QJsonDocument::fromJson(reply->readAll());
                    const auto object = doc.object();
                    const auto accessToken = object.value("access_token").toString().toLatin1();
                    const auto refreshToken = object.value("refresh_token").toString();

                    if (refreshToken.isEmpty())
                    {
                        qDebug() << "Empty refresh token!";
                        emit tokenError(tr("Empty refresh token!"));
                        return;
                    }

                    auto charReply = mNetworkManager.get(getVerifyRequest(accessToken));
                    connect(charReply, &QNetworkReply::finished, this, [=] {
                        BOOST_SCOPE_EXIT(this_) {
                            this_->mFetchingToken = false;
                        } BOOST_SCOPE_EXIT_END

                        charReply->deleteLater();

                        if (charReply->error() != QNetworkReply::NoError)
                        {
                            qDebug() << "Error verifying access token:" << charReply->errorString();
                            emit tokenError(charReply->errorString());
                            return;
                        }

                        const auto doc = QJsonDocument::fromJson(charReply->readAll());
                        const auto object = doc.object();
                        const auto realCharId = object["CharacterID"].toInt(Character::invalidId);

                        mRefreshTokens[realCharId] = refreshToken;

                        QSettings settings;
                        settings.setValue(SSOSettings::refreshTokenKey.arg(realCharId), mCrypt.encryptToByteArray(refreshToken));

                        if (charId != realCharId)
                        {
                            qDebug() << "Logged as invalid character id:" << realCharId;
                            emit tokenError(tr("Please authorize access for character: %1").arg(mCharacterRepo.getName(charId)));
                            return;
                        }

                        emit acquiredToken(realCharId, accessToken,
                                           QDateTime::currentDateTime().addSecs(object.value("expires_in").toInt() - 10));
                    });
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

    QNetworkRequest ESIManager::getAuthRequest() const
    {
        QNetworkRequest request{loginUrl + "/oauth/token"};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        request.setRawHeader(
            "Authorization", "Basic " + (mClientId + ":" + mClientSecret).toBase64());

        return request;
    }

    ExternalOrder ESIManager::getOrderFromJson(const QJsonObject &object, uint regionId) const
    {
        const auto range = object.value("range").toString();

        auto issued = QDateTime::fromString(object.value("issued").toString(), Qt::ISODate);
        issued.setTimeSpec(Qt::UTC);

        ExternalOrder order;

        order.setId(object.value("order_id").toDouble()); // https://bugreports.qt.io/browse/QTBUG-28560
        order.setType((object.value("is_buy_order").toBool()) ? (ExternalOrder::Type::Buy) : (ExternalOrder::Type::Sell));
        order.setTypeId(object.value("type_id").toDouble());
        order.setStationId(object.value("location_id").toDouble());

        //TODO: replace when available
        order.setSolarSystemId(mDataProvider.getStationSolarSystemId(order.getStationId()));
        order.setRegionId(regionId);

        if (range == "station")
            order.setRange(ExternalOrder::rangeStation);
        else if (range == "system")
            order.setRange(ExternalOrder::rangeSystem);
        else if (range == "region")
            order.setRange(ExternalOrder::rangeRegion);
        else
            order.setRange(range.toShort());

        order.setUpdateTime(QDateTime::currentDateTimeUtc());
        order.setPrice(object.value("price").toDouble());
        order.setVolumeEntered(object.value("volume_total").toInt());
        order.setVolumeRemaining(object.value("volume_remain").toInt());
        order.setMinVolume(object.value("min_volume").toInt());
        order.setIssued(issued);
        order.setDuration(object.value("duration").toInt());

        return order;
    }

    ESIInterface::PaginatedCallback ESIManager::getMarketOrderCallback(uint regionId, const MarketOrderCallback &callback) const
    {
        auto orders = std::make_shared<std::vector<ExternalOrder>>();
        return [=, orders = std::move(orders)](auto &&data, auto atEnd, const auto &error) {
            if (!error.isEmpty())
            {
                callback(std::vector<ExternalOrder>{}, error);
                return;
            }

            const auto items = data.array();
            const auto curSize = orders->size();
            orders->resize(curSize + items.size());

            std::atomic_size_t nextIndex{curSize};

            const auto parseItem = [&](const auto &item) {
                (*orders)[nextIndex++] = getOrderFromJson(item.toObject(), regionId);
            };

            QtConcurrent::blockingMap(items, parseItem);

            if (atEnd)
                callback(std::move(*orders), QString{});
        };
    }

    void ESIManager::createInterfaces()
    {
        QSettings settings;

        // IO bound
        const auto maxInterfaces = std::max(
            settings.value(NetworkSettings::maxESIThreadsKey, NetworkSettings::maxESIThreadsDefault).toUInt(),
            1u
        );

        mInterfaces.reserve(maxInterfaces);

        for (auto i = 0u; i < maxInterfaces; ++i)
        {
            const auto interface = new ESIInterface{this};

            mInterfaces.emplace_back(interface);

            connect(interface, &ESIInterface::tokenRequested, this, &ESIManager::fetchToken, Qt::QueuedConnection);
            connect(this, &ESIManager::acquiredToken, interface, &ESIInterface::updateTokenAndContinue);
            connect(this, &ESIManager::tokenError, interface, &ESIInterface::handleTokenError);
        }
    }

    const ESIInterface &ESIManager::selectNextInterface() const
    {
        const auto &interface = *mInterfaces[mCurrentInterface];
        mCurrentInterface = (mCurrentInterface + 1) % mInterfaces.size();

        return interface;
    }

    QNetworkRequest ESIManager::getVerifyRequest(const QByteArray &accessToken)
    {
        QNetworkRequest request{loginUrl + "/oauth/verify"};
        request.setRawHeader("Authorization", "Bearer  " + accessToken);

        return request;
    }
}
