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

        connect(this, &ESIManager::tokenError, this, [=](auto charId, const auto &errorInfo) {
            Q_UNUSED(charId);
            emit error(errorInfo);
        });
    }

    void ESIManager::fetchMarketOrders(uint regionId,
                                       EveType::IdType typeId,
                                       const MarketOrderCallback &callback) const
    {
        qDebug() << "Started market order import at" << QDateTime::currentDateTime();

        auto ifaceCallback = [=](auto &&data, const auto &error, const auto &expires) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error, expires);
                return;
            }

            const auto watcher = new QFutureWatcher<ExternalOrderList>{};
            connect(watcher, &QFutureWatcher<ExternalOrderList>::finished, this, [=] {
                watcher->deleteLater();

                const auto future = watcher->future();
                callback(future.result(), {}, expires);
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
        selectNextInterface().fetchMarketHistory(regionId, typeId, [=, callback = callback](auto &&data, const auto &error, const auto &expires) {
#else
        selectNextInterface().fetchMarketHistory(regionId, typeId, [=](auto &&data, const auto &error, const auto &expires) {
#endif
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error, expires);
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

            callback(QtConcurrent::blockingMappedReduced<HistoryMap>(data.array(), parseItem, insertItem), {}, expires);
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

    void ESIManager::fetchAssets(Character::IdType charId, const Callback<AssetList> &callback) const
    {
        selectNextInterface().fetchAssets(charId, [=](auto &&data, const auto &error, const auto &expires) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error, expires);
                return;
            }

            const auto assets = data.array();

            std::vector<AssetList::ItemType> allItems;
            allItems.reserve(assets.size());

            std::unordered_map<Item::IdType, Item *> itemMap;
            itemMap.reserve(assets.size());

            for (const auto &itemObj : assets)
            {
                const auto item = itemObj.toObject();

                auto newItem = std::make_unique<Item>(item.value(QStringLiteral("item_id")).toDouble());
                newItem->setLocationId(item.value(QStringLiteral("location_id")).toDouble());
                newItem->setTypeId(item.value(QStringLiteral("type_id")).toDouble());
                newItem->setQuantity(item.value(QStringLiteral("quantity")).toDouble());

                // ESI doesn't return raw quantity, so let's try to guess BPO/BPC status
                const auto name = mDataProvider.getTypeName(newItem->getTypeId());
                if (name.endsWith(QStringLiteral("Blueprint")))
                {
                    // BPC's are singletons (I hope...)
                    if (item.value(QStringLiteral("is_singleton")).toBool())
                        newItem->setRawQuantity(Item::magicBPCQuantity);
                    else
                        newItem->setRawQuantity(Item::magicBPOQuantity);
                }
                else
                {
                    newItem->setRawQuantity(newItem->getQuantity());
                }

                itemMap.emplace(newItem->getId(), newItem.get());
                allItems.emplace_back(std::move(newItem));
            }

            AssetList list;
            list.setCharacterId(charId);

            // make tree
            for (auto &item : allItems)
            {
                const auto parent = itemMap.find(*item->getLocationId());
                if (parent != std::end(itemMap))
                {
                    item->setLocationId({});
                    parent->second->addItem(std::move(item));
                }
                else
                {
                    list.addItem(std::move(item));
                }
            }

            callback(std::move(list), {}, expires);
        });
    }

    void ESIManager::fetchCharacter(Character::IdType charId, const Callback<Character> &callback) const
    {
        selectNextInterface().fetchCharacter(charId, [=](auto &&publicData, const auto &error, const auto &expires) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error, expires);
                return;
            }

            selectNextInterface().fetchCharacterSkills(
                charId, [=, publicData = std::move(publicData)](auto &&skillData, const auto &error, const auto &expires) {
                    if (Q_UNLIKELY(!error.isEmpty()))
                    {
                        callback({}, error, expires);
                        return;
                    }

                    const auto publicDataObj = publicData.object();

                    selectNextInterface().fetchCorporation(
                        publicDataObj.value(QStringLiteral("corporation_id")).toDouble(),
                        [=, publicDataObj = std::move(publicDataObj), skillData = std::move(skillData)](auto &&corpData, const auto &error, const auto &expires) {
                            if (Q_UNLIKELY(!error.isEmpty()))
                            {
                                callback({}, error, expires);
                                return;
                            }

                            selectNextInterface().fetchCharacterWallet(
                                charId,
                                [=, corpData = std::move(corpData), publicDataObj = std::move(publicDataObj), skillData = std::move(skillData)](auto &&walletData, const auto &error, const auto &expires) {
                                    if (Q_UNLIKELY(!error.isEmpty()))
                                    {
                                        callback({}, error, expires);
                                        return;
                                    }

                                    const auto corpDataObj = corpData.object();

                                    Character character{charId};
                                    character.setName(publicDataObj.value(QStringLiteral("name")).toString());
                                    character.setCorporationName(corpDataObj.value(QStringLiteral("corporation_name")).toString());
                                    character.setCorporationId(publicDataObj.value(QStringLiteral("corporation_id")).toDouble());
                                    character.setRace(mDataProvider.getRaceName(publicDataObj.value(QStringLiteral("race_id")).toDouble()));
                                    character.setBloodline(mDataProvider.getBloodlineName(publicDataObj.value(QStringLiteral("bloodline_id")).toDouble()));
                                    // TODO: change when ancestry enpoint becomes available
                                    character.setAncestry(mDataProvider.getGenericName(publicDataObj.value(QStringLiteral("ancestry_id")).toDouble()));
                                    character.setGender(publicDataObj.value(QStringLiteral("gender")).toString());
                                    character.setISK(walletData.toDouble());

                                    CharacterData::OrderAmountSkills orderAmountSkills;
                                    CharacterData::TradeRangeSkills tradeRangeSkills;
                                    CharacterData::FeeSkills feeSkills;
                                    CharacterData::ContractSkills contractSkills;
                                    CharacterData::ReprocessingSkills reprocessingSkills;

                                    const auto skills = skillData.object().value(QStringLiteral("skills")).toArray();
                                    for (const auto &skill : skills)
                                    {
                                        const auto skillObj = skill.toObject();
                                        switch (skillObj.value(QStringLiteral("skill_id")).toInt()) {
                                        case 3443:
                                            orderAmountSkills.mTrade = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 3444:
                                            orderAmountSkills.mRetail = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 16596:
                                            orderAmountSkills.mWholesale = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 18580:
                                            orderAmountSkills.mTycoon = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 16598:
                                            tradeRangeSkills.mMarketing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 16594:
                                            tradeRangeSkills.mProcurement = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 16595:
                                            tradeRangeSkills.mDaytrading = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 3447:
                                            tradeRangeSkills.mVisibility = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 16622:
                                            feeSkills.mAccounting = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 3446:
                                            feeSkills.mBrokerRelations = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 16597:
                                            feeSkills.mMarginTrading = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 25235:
                                            contractSkills.mContracting = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12180:
                                            reprocessingSkills.mArkonorProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12181:
                                            reprocessingSkills.mBistotProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12182:
                                            reprocessingSkills.mCrokiteProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12183:
                                            reprocessingSkills.mDarkOchreProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12185:
                                            reprocessingSkills.mHedbergiteProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12186:
                                            reprocessingSkills.mHemorphiteProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 18025:
                                            reprocessingSkills.mIceProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12187:
                                            reprocessingSkills.mJaspetProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12188:
                                            reprocessingSkills.mKerniteProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12189:
                                            reprocessingSkills.mMercoxitProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12190:
                                            reprocessingSkills.mOmberProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12191:
                                            reprocessingSkills.mPlagioclaseProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12192:
                                            reprocessingSkills.mPyroxeresProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 3385:
                                            reprocessingSkills.mReprocessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 3389:
                                            reprocessingSkills.mReprocessingEfficiency = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12193:
                                            reprocessingSkills.mScorditeProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12196:
                                            reprocessingSkills.mScrapmetalProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12194:
                                            reprocessingSkills.mSpodumainProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 12195:
                                            reprocessingSkills.mVeldsparProcessing = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                        }
                                    }

                                    character.setOrderAmountSkills(std::move(orderAmountSkills));
                                    character.setTradeRangeSkills(std::move(tradeRangeSkills));
                                    character.setFeeSkills(std::move(feeSkills));
                                    character.setContractSkills(std::move(contractSkills));
                                    character.setReprocessingSkills(std::move(reprocessingSkills));

                                    callback(std::move(character), {}, expires);
                            });
                    });
            });
        });
    }

    void ESIManager::fetchRaces(const Callback<NameMap> &callback) const
    {
        selectNextInterface().fetchRaces([=](auto &&data, const auto &error, const auto &expires) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error, expires);
                return;
            }

            const auto races = data.array();

            NameMap names;
            names.reserve(races.size());

            for (const auto &race : races)
            {
                const auto raceObj = race.toObject();
                names.emplace(
                    static_cast<quint64>(raceObj.value(QStringLiteral("race_id")).toDouble()),
                    raceObj.value(QStringLiteral("name")).toString()
                );
            }

            callback(std::move(names), {}, expires);
        });
    }

    void ESIManager::fetchBloodlines(const Callback<NameMap> &callback) const
    {
        selectNextInterface().fetchBloodlines([=](auto &&data, const auto &error, const auto &expires) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error, expires);
                return;
            }

            const auto bloodlines = data.array();

            NameMap names;
            names.reserve(bloodlines.size());

            for (const auto &bloodline : bloodlines)
            {
                const auto bloodlineObj = bloodline.toObject();
                names.emplace(
                    static_cast<quint64>(bloodlineObj.value(QStringLiteral("bloodline_id")).toDouble()),
                    bloodlineObj.value(QStringLiteral("name")).toString()
                );
            }

            callback(std::move(names), {}, expires);
        });
    }

    void ESIManager::fetchCharacterMarketOrders(Character::IdType charId, const Callback<MarketOrders> &callback) const
    {
        selectNextInterface().fetchCharacterMarketOrders(charId, [=](auto &&data, const auto &error, const auto &expires) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error, expires);
                return;
            }

            const auto orderArray = data.array();
            MarketOrders orders(orderArray.size());

            std::atomic_size_t index{0};
            QtConcurrent::blockingMap(orderArray, [&, charId](const auto &order) {
                const auto orderObj = order.toObject();

                auto issued = QDateTime::fromString(orderObj.value("issued").toString(), Qt::ISODate);
                issued.setTimeSpec(Qt::UTC);

                auto &curOrder = orders[index++];
                curOrder.setId(orderObj.value(QStringLiteral("order_id")).toDouble());
                curOrder.setCharacterId(charId);
                curOrder.setStationId(orderObj.value(QStringLiteral("location_id")).toDouble());
                curOrder.setVolumeEntered(orderObj.value(QStringLiteral("volume_total")).toDouble());
                curOrder.setVolumeRemaining(orderObj.value(QStringLiteral("volume_remain")).toDouble());
                curOrder.setMinVolume(orderObj.value(QStringLiteral("min_volume")).toDouble());
                curOrder.setState(getStateFromString(orderObj.value(QStringLiteral("state")).toString()));
                curOrder.setTypeId(orderObj.value(QStringLiteral("type_id")).toDouble());
                curOrder.setRange(getMarketOrderRangeFromString(orderObj.value(QStringLiteral("range")).toString()));
                curOrder.setDuration(orderObj.value(QStringLiteral("duration")).toInt());
                curOrder.setEscrow(orderObj.value(QStringLiteral("escrow")).toDouble());
                curOrder.setPrice(orderObj.value(QStringLiteral("price")).toDouble());
                curOrder.setType((orderObj.value(QStringLiteral("is_buy_order")).toBool()) ? (MarketOrder::Type::Buy) : (MarketOrder::Type::Sell));
                curOrder.setIssued(issued);
                curOrder.setFirstSeen(issued);
            });

            callback(std::move(orders), {}, expires);
        });
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
        mPendingTokenRefresh.insert(charId);

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
                query.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
                query.addQueryItem(QStringLiteral("redirect_uri"), QStringLiteral("http://%1/sso-authentication/").arg(redirectDomain));
                query.addQueryItem(QStringLiteral("client_id"), mClientId);
                query.addQueryItem(
                    QStringLiteral("scope"),
                    QStringLiteral(
                        "esi-skills.read_skills.v1 "
                        "esi-wallet.read_character_wallet.v1 "
                        "esi-assets.read_assets.v1 "
                        "esi-ui.open_window.v1 "
                        "esi-ui.write_waypoint.v1 "
                        "esi-markets.structure_markets.v1 "
                        "esi-markets.read_character_orders.v1"
                    )
                );

                url.setQuery(query);

                mAuthView.reset(new SSOAuthWidget{url});

                QString charName;

                try
                {
                    charName = mCharacterRepo.getName(charId);
                }
                catch (const CharacterRepository::NotFoundException &)
                {
                    charName = mDataProvider.getGenericName(charId);
                }

                if (charName.isEmpty())
                    mAuthView->setWindowTitle(tr("SSO Authentication for unknown character: %1").arg(charId));
                else
                    mAuthView->setWindowTitle(tr("SSO Authentication for character: %1").arg(charName));

                mAuthView->setWindowModality(Qt::ApplicationModal);
                mAuthView->adjustSize();
                mAuthView->move(QApplication::desktop()->screenGeometry(QApplication::activeWindow()).center() -
                                mAuthView->rect().center());
                mAuthView->show();

                connect(mAuthView.data(), &SSOAuthWidget::acquiredCode, this, [=](const auto &code) {
                    processAuthorizationCode(charId, code);
                });
                connect(mAuthView.data(), &SSOAuthWidget::aboutToClose, this, [=] {
                    mPendingTokenRefresh.erase(charId);
                    mFetchingToken = false;

                    qDebug() << "Auth window closed.";
                    emit tokenError(charId, tr("SSO authorization failed."));

                    scheduleNextTokenFetch();
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
                        mPendingTokenRefresh.clear();
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

                        if (Q_UNLIKELY(reply->error() != QNetworkReply::NoError))
                        {
                            qWarning() << "Error refreshing token:" << reply->errorString();

                            const auto error = object.value("error").toString();

                            qWarning() << "Returned error:" << error;

                            mPendingTokenRefresh.erase(charId);
                            mFetchingToken = false;

                            if (error == "invalid_token" || error == "invalid_client" || error == "invalid_grant")
                            {
                                fetchToken(charId);
                            }
                            else
                            {
                                const auto desc = object.value("error_description").toString();
                                emit tokenError(charId, (desc.isEmpty()) ? (reply->errorString()) : (desc));

                                scheduleNextTokenFetch();
                            }

                            return;
                        }

                        BOOST_SCOPE_EXIT(this_, charId) {
                            this_->mPendingTokenRefresh.erase(charId);
                            this_->mFetchingToken = false;
                            this_->scheduleNextTokenFetch();
                        } BOOST_SCOPE_EXIT_END

                        const auto accessToken = object.value("access_token").toString();
                        if (accessToken.isEmpty())
                        {
                            qWarning() << "Empty access token!";
                            emit tokenError(charId, tr("Empty access token!"));
                            return;
                        }

                        emit acquiredToken(charId, accessToken,
                                           QDateTime::currentDateTime().addSecs(doc.object().value("expires_in").toInt() - 10));
                    }
                    catch (...)
                    {
                        mPendingTokenRefresh.clear();
                        mFetchingToken = false;
                        throw;
                    }
                });
            }
        }
        catch (...)
        {
            mPendingTokenRefresh.clear();
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
            mAuthView->disconnect(this);
            mAuthView->close();

            qDebug() << "Requesting access token...";

            QByteArray data = "grant_type=authorization_code&code=" + code;

            auto reply = mNetworkManager.post(getAuthRequest(), data);
            connect(reply, &QNetworkReply::finished, this, [=] {
                try
                {
                    mPendingTokenRefresh.erase(charId);

                    reply->deleteLater();

                    if (Q_UNLIKELY(reply->error() != QNetworkReply::NoError))
                    {
                        mFetchingToken = false;

                        qDebug() << "Error requesting access token:" << reply->errorString();
                        emit tokenError(charId, reply->errorString());

                        scheduleNextTokenFetch();
                        return;
                    }

                    const auto doc = QJsonDocument::fromJson(reply->readAll());
                    const auto object = doc.object();
                    const auto accessToken = object.value("access_token").toString().toLatin1();
                    const auto refreshToken = object.value("refresh_token").toString();

                    if (Q_UNLIKELY(refreshToken.isEmpty()))
                    {
                        mFetchingToken = false;

                        qDebug() << "Empty refresh token!";
                        emit tokenError(charId, tr("Empty refresh token!"));

                        scheduleNextTokenFetch();
                        return;
                    }

                    auto charReply = mNetworkManager.get(getVerifyRequest(accessToken));
                    connect(charReply, &QNetworkReply::finished, this, [=] {
                        BOOST_SCOPE_EXIT(this_) {
                            this_->mFetchingToken = false;
                            this_->scheduleNextTokenFetch();
                        } BOOST_SCOPE_EXIT_END

                        charReply->deleteLater();

                        if (Q_UNLIKELY(charReply->error() != QNetworkReply::NoError))
                        {
                            qDebug() << "Error verifying access token:" << charReply->errorString();
                            emit tokenError(charId, charReply->errorString());
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
                            emit tokenError(charId, tr("Please authorize access for character: %1").arg(mCharacterRepo.getName(charId)));
                            return;
                        }

                        emit acquiredToken(realCharId, accessToken,
                                           QDateTime::currentDateTime().addSecs(object.value("expires_in").toInt() - 10));
                    });
                }
                catch (...)
                {
                    mPendingTokenRefresh.clear();
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
        return [=, orders = std::move(orders)](auto &&data, auto atEnd, const auto &error, const auto &expires) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error, expires);
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
                callback(std::move(*orders), {}, expires);
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

    void ESIManager::scheduleNextTokenFetch()
    {
        if (mPendingTokenRefresh.empty())
            return;

        const auto charId = *std::begin(mPendingTokenRefresh);
        mPendingTokenRefresh.erase(std::begin(mPendingTokenRefresh));

        QMetaObject::invokeMethod(this, "fetchToken", Q_ARG(Character::IdType, charId));
    }

    const ESIInterface &ESIManager::selectNextInterface() const
    {
        const auto &interface = *mInterfaces[mCurrentInterface];
        mCurrentInterface = (mCurrentInterface + 1) % mInterfaces.size();

        return interface;
    }

    MarketOrder::State ESIManager::getStateFromString(const QString &state)
    {
        static const QHash<QString, MarketOrder::State> states = {
            { QStringLiteral("open"), MarketOrder::State::Active },
            { QStringLiteral("closed"), MarketOrder::State::Closed },
            { QStringLiteral("expired"), MarketOrder::State::Fulfilled },
            { QStringLiteral("cancelled"), MarketOrder::State::Cancelled },
            { QStringLiteral("pending"), MarketOrder::State::Pending },
            { QStringLiteral("character_deleted"), MarketOrder::State::CharacterDeleted },
        };

        // assume unknown is active, since there shouldn't be non-open orders returned anyway
        return (states.contains(state)) ? (states[state]) : (MarketOrder::State::Active);
    }

    short ESIManager::getMarketOrderRangeFromString(const QString &range)
    {
        static const QHash<QString, short> ranges = {
            { QStringLiteral("station"), MarketOrder::rangeStation },
            { QStringLiteral("region"), MarketOrder::rangeRegion },
            { QStringLiteral("solarsystem"), MarketOrder::rangeSystem },
        };

        // assume unknown is active, since there shouldn't be non-open orders returned anyway
        return (ranges.contains(range)) ? (ranges[range]) : (range.toShort());
    }

    QNetworkRequest ESIManager::getVerifyRequest(const QByteArray &accessToken)
    {
        QNetworkRequest request{loginUrl + "/oauth/verify"};
        request.setRawHeader("Authorization", "Bearer  " + accessToken);

        return request;
    }
}
