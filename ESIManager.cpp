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
#include <limits>
#include <mutex>

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
#include <QDebug>
#include <QHash>

#include <boost/scope_exit.hpp>

#include "CharacterRepository.h"
#include "EveDataProvider.h"
#include "NetworkSettings.h"
#include "ExternalOrder.h"
#include "ReplyTimeout.h"
#include "SSOSettings.h"
#include "Blueprint.h"
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

        connect(this, &ESIManager::tokenError, this, [=](auto charId, const auto &errorInfo) {
            Q_UNUSED(charId);
            emit error(errorInfo);
        });

        connect(&mInterfaceManager, &ESIInterfaceManager::tokenRequested, this, &ESIManager::fetchToken);
        connect(this, &ESIManager::acquiredToken, &mInterfaceManager, &ESIInterfaceManager::acquiredToken);
        connect(this, &ESIManager::tokenError, &mInterfaceManager, &ESIInterfaceManager::tokenError);
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
                    result.emplace_back(getExternalOrderFromJson(item.toObject(), regionId));

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
                auto date = QDate::fromString(itemObject.value(QStringLiteral("date")).toString(), Qt::ISODate);

                MarketHistoryEntry entry;
                entry.mAvgPrice = itemObject.value(QStringLiteral("average")).toDouble();
                entry.mHighPrice = itemObject.value(QStringLiteral("highest")).toDouble();
                entry.mLowPrice = itemObject.value(QStringLiteral("lowest")).toDouble();
                entry.mOrders = itemObject.value(QStringLiteral("order_count")).toInt();
                entry.mVolume = itemObject.value(QStringLiteral("volume")).toDouble();

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

    void ESIManager::fetchCharacterAssets(Character::IdType charId, const Callback<AssetList> &callback) const
    {
        selectNextInterface().fetchCharacterAssets(charId, [=](auto &&data, const auto &error, const auto &expires) {
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
                const int rawQuantity = item.value(QStringLiteral("quantity")).toDouble();

                auto newItem = std::make_unique<Item>(static_cast<Item::IdType>(item.value(QStringLiteral("item_id")).toDouble()));
                newItem->setLocationId(item.value(QStringLiteral("location_id")).toDouble());
                newItem->setTypeId(item.value(QStringLiteral("type_id")).toDouble());
                newItem->setRawQuantity(rawQuantity);
                // https://forums.eveonline.com/t/esi-assets-blueprints-and-quantities/19345/4
                newItem->setQuantity((rawQuantity < 0) ? (1) : (rawQuantity));

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
                                    CharacterData::IndustrySkills industrySkills;

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
                                            break;
                                        case 3380:
                                            industrySkills.mIndustry = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 3388:
                                            industrySkills.mAdvancedIndustry = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 3398:
                                            industrySkills.mAdvancedLargeShipConstruction = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 3397:
                                            industrySkills.mAdvancedMediumShipConstruction = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 3395:
                                            industrySkills.mAdvancedSmallShipConstruction = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11444:
                                            industrySkills.mAmarrStarshipEngineering = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 3396:
                                            industrySkills.mAvancedIndustrialShipConstruction = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11454:
                                            industrySkills.mCaldariStarshipEngineering = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11448:
                                            industrySkills.mElectromagneticPhysics = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11453:
                                            industrySkills.mElectronicEngineering = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11450:
                                            industrySkills.mGallenteStarshipEngineering = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11446:
                                            industrySkills.mGravitonPhysics = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11433:
                                            industrySkills.mHighEnergyPhysics = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11443:
                                            industrySkills.mHydromagneticPhysics = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11447:
                                            industrySkills.mLaserPhysics = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11452:
                                            industrySkills.mMechanicalEngineering = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11445:
                                            industrySkills.mMinmatarStarshipEngineering = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11529:
                                            industrySkills.mMolecularEngineering = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11451:
                                            industrySkills.mNuclearPhysics = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11441:
                                            industrySkills.mPlasmaPhysics = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11455:
                                            industrySkills.mQuantumPhysics = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                            break;
                                        case 11449:
                                            industrySkills.mRocketScience = skillObj.value(QStringLiteral("current_skill_level")).toInt();
                                        }
                                    }

                                    character.setOrderAmountSkills(std::move(orderAmountSkills));
                                    character.setTradeRangeSkills(std::move(tradeRangeSkills));
                                    character.setFeeSkills(std::move(feeSkills));
                                    character.setContractSkills(std::move(contractSkills));
                                    character.setReprocessingSkills(std::move(reprocessingSkills));
                                    character.setIndustrySkills(std::move(industrySkills));

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

                const auto issued = getDateTimeFromString(orderObj.value("issued").toString());

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

    void ESIManager::fetchCharacterWalletJournal(Character::IdType charId,
                                                 WalletJournalEntry::IdType tillId,
                                                 const Callback<WalletJournal> &callback) const
    {
        fetchCharacterWalletJournal(charId, boost::none, tillId, callback);
    }

    void ESIManager::fetchCharacterWalletTransactions(Character::IdType charId,
                                                      WalletTransaction::IdType tillId,
                                                      const Callback<WalletTransactions> &callback) const
    {
        fetchCharacterWalletTransactions(charId, boost::none, tillId, std::make_shared<WalletTransactions>(), callback);
    }

    void ESIManager::fetchGenericName(quint64 id, const PesistentDataCallback<QString> &callback) const
    {
        selectNextInterface().fetchGenericName(id, [=](auto &&data, const auto &error) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error);
                return;
            }

            callback(std::move(data), error);
        });
    }

    void ESIManager::fetchCharacterBlueprints(Character::IdType charId, const Callback<BlueprintList> &callback) const
    {
        selectNextInterface().fetchCharacterBlueprints(charId, [=](auto &&data, const auto &error, const auto &expires) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error, expires);
                return;
            }

            const auto blueprintList = data.array();

            BlueprintList result(blueprintList.size());
            std::atomic_size_t currentIndex{0};

            const QHash<QString, Blueprint::Location> locationNameMap = {
                { QStringLiteral("AutoFit"), Blueprint::Location::AutoFit },
                { QStringLiteral("Cargo"), Blueprint::Location::Cargo },
                { QStringLiteral("CorpseBay"), Blueprint::Location::CorpseBay },
                { QStringLiteral("DroneBay"), Blueprint::Location::DroneBay },
                { QStringLiteral("FleetHangar"), Blueprint::Location::FleetHangar },
                { QStringLiteral("Deliveries"), Blueprint::Location::Deliveries },
                { QStringLiteral("HiddenModifiers"), Blueprint::Location::HiddenModifiers },
                { QStringLiteral("Hangar"), Blueprint::Location::Hangar },
                { QStringLiteral("HangarAll"), Blueprint::Location::HangarAll },
                { QStringLiteral("LoSlot0"), Blueprint::Location::LoSlot0 },
                { QStringLiteral("LoSlot1"), Blueprint::Location::LoSlot1 },
                { QStringLiteral("LoSlot2"), Blueprint::Location::LoSlot2 },
                { QStringLiteral("LoSlot3"), Blueprint::Location::LoSlot3 },
                { QStringLiteral("LoSlot4"), Blueprint::Location::LoSlot4 },
                { QStringLiteral("LoSlot5"), Blueprint::Location::LoSlot5 },
                { QStringLiteral("LoSlot6"), Blueprint::Location::LoSlot6 },
                { QStringLiteral("LoSlot7"), Blueprint::Location::LoSlot7 },
                { QStringLiteral("MedSlot0"), Blueprint::Location::MedSlot0 },
                { QStringLiteral("MedSlot1"), Blueprint::Location::MedSlot1 },
                { QStringLiteral("MedSlot2"), Blueprint::Location::MedSlot2 },
                { QStringLiteral("MedSlot3"), Blueprint::Location::MedSlot3 },
                { QStringLiteral("MedSlot4"), Blueprint::Location::MedSlot4 },
                { QStringLiteral("MedSlot5"), Blueprint::Location::MedSlot5 },
                { QStringLiteral("MedSlot6"), Blueprint::Location::MedSlot6 },
                { QStringLiteral("MedSlot7"), Blueprint::Location::MedSlot7 },
                { QStringLiteral("HiSlot0"), Blueprint::Location::HiSlot0 },
                { QStringLiteral("HiSlot1"), Blueprint::Location::HiSlot1 },
                { QStringLiteral("HiSlot2"), Blueprint::Location::HiSlot2 },
                { QStringLiteral("HiSlot3"), Blueprint::Location::HiSlot3 },
                { QStringLiteral("HiSlot4"), Blueprint::Location::HiSlot4 },
                { QStringLiteral("HiSlot5"), Blueprint::Location::HiSlot5 },
                { QStringLiteral("HiSlot6"), Blueprint::Location::HiSlot6 },
                { QStringLiteral("HiSlot7"), Blueprint::Location::HiSlot7 },
                { QStringLiteral("AssetSafety"), Blueprint::Location::AssetSafety },
                { QStringLiteral("Locked"), Blueprint::Location::Locked },
                { QStringLiteral("Unlocked"), Blueprint::Location::Unlocked },
                { QStringLiteral("Implant"), Blueprint::Location::Implant },
                { QStringLiteral("QuafeBay"), Blueprint::Location::QuafeBay },
                { QStringLiteral("RigSlot0"), Blueprint::Location::RigSlot0 },
                { QStringLiteral("RigSlot1"), Blueprint::Location::RigSlot1 },
                { QStringLiteral("RigSlot2"), Blueprint::Location::RigSlot2 },
                { QStringLiteral("RigSlot3"), Blueprint::Location::RigSlot3 },
                { QStringLiteral("RigSlot4"), Blueprint::Location::RigSlot4 },
                { QStringLiteral("RigSlot5"), Blueprint::Location::RigSlot5 },
                { QStringLiteral("RigSlot6"), Blueprint::Location::RigSlot6 },
                { QStringLiteral("RigSlot7"), Blueprint::Location::RigSlot7 },
                { QStringLiteral("ShipHangar"), Blueprint::Location::ShipHangar },
                { QStringLiteral("SpecializedFuelBay"), Blueprint::Location::SpecializedFuelBay },
                { QStringLiteral("SpecializedOreHold"), Blueprint::Location::SpecializedOreHold },
                { QStringLiteral("SpecializedGasHold"), Blueprint::Location::SpecializedGasHold },
                { QStringLiteral("SpecializedMineralHold"), Blueprint::Location::SpecializedMineralHold },
                { QStringLiteral("SpecializedSalvageHold"), Blueprint::Location::SpecializedSalvageHold },
                { QStringLiteral("SpecializedShipHold"), Blueprint::Location::SpecializedShipHold },
                { QStringLiteral("SpecializedSmallShipHold"), Blueprint::Location::SpecializedSmallShipHold },
                { QStringLiteral("SpecializedMediumShipHold"), Blueprint::Location::SpecializedMediumShipHold },
                { QStringLiteral("SpecializedLargeShipHold"), Blueprint::Location::SpecializedLargeShipHold },
                { QStringLiteral("SpecializedIndustrialShipHold"), Blueprint::Location::SpecializedIndustrialShipHold },
                { QStringLiteral("SpecializedAmmoHold"), Blueprint::Location::SpecializedAmmoHold },
                { QStringLiteral("SpecializedCommandCenterHold"), Blueprint::Location::SpecializedCommandCenterHold },
                { QStringLiteral("SpecializedPlanetaryCommoditiesHold"), Blueprint::Location::SpecializedPlanetaryCommoditiesHold },
                { QStringLiteral("SpecializedMaterialBay"), Blueprint::Location::SpecializedMaterialBay },
                { QStringLiteral("SubSystemSlot0"), Blueprint::Location::SubSystemSlot0 },
                { QStringLiteral("SubSystemSlot1"), Blueprint::Location::SubSystemSlot1 },
                { QStringLiteral("SubSystemSlot2"), Blueprint::Location::SubSystemSlot2 },
                { QStringLiteral("SubSystemSlot3"), Blueprint::Location::SubSystemSlot3 },
                { QStringLiteral("SubSystemSlot4"), Blueprint::Location::SubSystemSlot4 },
                { QStringLiteral("SubSystemSlot5"), Blueprint::Location::SubSystemSlot5 },
                { QStringLiteral("SubSystemSlot6"), Blueprint::Location::SubSystemSlot6 },
                { QStringLiteral("SubSystemSlot7"), Blueprint::Location::SubSystemSlot7 },
                { QStringLiteral("FighterBay"), Blueprint::Location::FighterBay },
                { QStringLiteral("FighterTube0"), Blueprint::Location::FighterTube0 },
                { QStringLiteral("FighterTube1"), Blueprint::Location::FighterTube1 },
                { QStringLiteral("FighterTube2"), Blueprint::Location::FighterTube2 },
                { QStringLiteral("FighterTube3"), Blueprint::Location::FighterTube3 },
                { QStringLiteral("FighterTube4"), Blueprint::Location::FighterTube4 },
                { QStringLiteral("Module"), Blueprint::Location::Module },
            };

            QtConcurrent::blockingMap(blueprintList, [&](const auto &blueprint) {
                const auto blueprintObj = blueprint.toObject();

                auto &curResult = result[currentIndex++];
                curResult.setId(blueprintObj.value(QStringLiteral("item_id")).toDouble());
                curResult.setLocationId(blueprintObj.value(QStringLiteral("location_id")).toDouble());
                curResult.setMaterialEfficiency(blueprintObj.value(QStringLiteral("material_efficiency")).toDouble());
                curResult.setTimeEfficiency(blueprintObj.value(QStringLiteral("time_efficiency")).toDouble());
                curResult.setQuantity(blueprintObj.value(QStringLiteral("quantity")).toInt());
                curResult.setRuns(blueprintObj.value(QStringLiteral("runs")).toInt());
                curResult.setTypeId(blueprintObj.value(QStringLiteral("type_id")).toDouble());

                const auto location = blueprintObj.value(QStringLiteral("location_flag")).toString();
                if (locationNameMap.contains(location))
                    curResult.setLocation(locationNameMap[location]);
            });

            callback(std::move(result), error, expires);
        });
    }

    void ESIManager::fetchMarketPrices(const PesistentDataCallback<MarketPrices> &callback) const
    {
        selectNextInterface().fetchMarketPrices([=](auto &&data, const auto &error) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error);
                return;
            }

            const auto prices = data.array();

            MarketPrices result;
            result.reserve(prices.size());

            for (const auto &price : prices)
            {
                const auto priceObj = price.toObject();
                result.emplace(static_cast<EveType::IdType>(priceObj.value(QStringLiteral("type_id")).toDouble()), TypePriceData{
                    priceObj.value(QStringLiteral("adjusted_price")).toDouble(),
                    priceObj.value(QStringLiteral("average_price")).toDouble()
                });
            }

            callback(std::move(result), {});
        });
    }

    void ESIManager::fetchIndustryCostIndices(const PesistentDataCallback<IndustryCostIndices> &callback) const
    {
        selectNextInterface().fetchIndustryCostIndices([=](auto &&data, const auto &error) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error);
                return;
            }

            const auto prices = data.array();

            IndustryCostIndices result;
            result.reserve(prices.size());

            std::mutex resultMutex;

            const QHash<QString, IndustryCostIndex::Activity> activityNameMap = {
                { QStringLiteral("none"), IndustryCostIndex::Activity::None },
                { QStringLiteral("manufacturing"), IndustryCostIndex::Activity::Manufacturing },
                { QStringLiteral("researching_technology"), IndustryCostIndex::Activity::ResearchingTechnology },
                { QStringLiteral("researching_time_efficiency"), IndustryCostIndex::Activity::ResearchingTimeEfficiency },
                { QStringLiteral("researching_material_efficiency"), IndustryCostIndex::Activity::ResearchingMaterialEfficiency },
                { QStringLiteral("copying"), IndustryCostIndex::Activity::Copying },
                { QStringLiteral("duplicating"), IndustryCostIndex::Activity::Duplicating },
                { QStringLiteral("invention"), IndustryCostIndex::Activity::Invention },
                { QStringLiteral("reverse_engineering"), IndustryCostIndex::Activity::ReverseEngineering },
            };

            QtConcurrent::blockingMap(
                prices,
                std::function<void (const QJsonValue &)>{[&](const auto &index) {
                    const auto indexObj = index.toObject();
                    const auto costs = indexObj.value(QStringLiteral("cost_indices")).toArray();

                    const uint systemId = indexObj.value(QStringLiteral("solar_system_id")).toDouble();

                    IndustryCostIndex::ActivityCostIndices indices;
                    indices.reserve(costs.size());

                    for (const auto &cost : costs)
                    {
                        const auto costObj = cost.toObject();
                        const auto activityStr = costObj.value(QStringLiteral("activity")).toString();
                        const auto value = costObj.value(QStringLiteral("cost_index")).toDouble();
                        const auto activity = activityNameMap.value(activityStr, IndustryCostIndex::Activity::None);

                        indices.emplace(activity, value);
                    }

                    std::lock_guard<std::mutex> lock{resultMutex};
                    result.emplace(systemId, std::move(indices));
                }}
            );

            callback(std::move(result), {});
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
                        "esi-markets.read_character_orders.v1 "
                        "esi-characters.read_blueprints.v1 "
                        "esi-contracts.read_character_contracts.v1"
                    )
                );

                url.setQuery(query);

                mAuthView.reset(new SSOAuthWidget{url});

                QString charName;
                auto charNameFound = false;

                std::tie(charName, charNameFound) = getCharacterName(charId);

                if (charName.isEmpty())
                    mAuthView->setWindowTitle(tr("SSO Authentication for unknown character: %1").arg(charId));
                else
                    mAuthView->setWindowTitle(getAuthWidowTitle(charName));

                mAuthView->setWindowModality(Qt::ApplicationModal);
                mAuthView->adjustSize();
                mAuthView->move(QApplication::desktop()->screenGeometry(mAuthView.get()).center() -
                                mAuthView->rect().center());
                mAuthView->show();

                connect(mAuthView.get(), &SSOAuthWidget::acquiredCode, this, [=](const auto &code) {
                    processAuthorizationCode(charId, code);
                });
                connect(mAuthView.get(), &SSOAuthWidget::aboutToClose, this, [=] {
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

                if (!charNameFound)
                {
                    connect(&mDataProvider, &EveDataProvider::namesChanged, mAuthView.get(), [=] {
                        mAuthView->setWindowTitle(getAuthWidowTitle(mDataProvider.getGenericName(charId)));
                    });
                }
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

                            const auto error = object.value(QStringLiteral("error")).toString();

                            qWarning() << "Returned error:" << error;

                            mPendingTokenRefresh.erase(charId);
                            mFetchingToken = false;

                            if (error == "invalid_token" || error == "invalid_client" || error == "invalid_grant")
                            {
                                mRefreshTokens.erase(charId);
                                fetchToken(charId);
                            }
                            else
                            {
                                const auto desc = object.value(QStringLiteral("error_description")).toString();
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

                        const auto accessToken = object.value(QStringLiteral("access_token")).toString();
                        if (accessToken.isEmpty())
                        {
                            qWarning() << "Empty access token!";
                            emit tokenError(charId, tr("Empty access token!"));
                            return;
                        }

                        emit acquiredToken(charId, accessToken,
                                           QDateTime::currentDateTime().addSecs(doc.object().value(QStringLiteral("expires_in")).toInt() - 10));
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
        mInterfaceManager.handleNewPreferences();
    }

    void ESIManager::fetchCharacterWalletJournal(Character::IdType charId,
                                                 const boost::optional<WalletJournalEntry::IdType> &fromId,
                                                 WalletJournalEntry::IdType tillId,
                                                 const Callback<WalletJournal> &callback) const
    {
        selectNextInterface().fetchCharacterWalletJournal(charId, fromId, [=](auto &&data, const auto &error, const auto &expires) {
            if (Q_UNLIKELY(!error.isEmpty()))
            {
                callback({}, error, expires);
                return;
            }

            // https://bugreports.qt.io/browse/QTBUG-61145
            auto journal = QtConcurrent::blockingMappedReduced<WalletJournal>(
                data.array(),
                std::function<WalletJournalEntry (const QJsonValue &)>{[=](const auto &value) {
                    const auto entryObj = value.toObject();

                    WalletJournalEntry entry{static_cast<WalletJournalEntry::IdType>(entryObj.value(QStringLiteral("ref_id")).toDouble())};
                    entry.setCharacterId(charId);
                    entry.setTimestamp(getDateTimeFromString(entryObj.value(QStringLiteral("date")).toString()));
                    entry.setRefType(entryObj.value(QStringLiteral("ref_type")).toString());

                    if (entryObj.contains(QStringLiteral("first_party_id")))
                        entry.setFirstPartyId(entryObj.value(QStringLiteral("first_party_id")).toDouble());
                    if (entryObj.contains(QStringLiteral("second_party_id")))
                        entry.setSecondPartyId(entryObj.value(QStringLiteral("second_party_id")).toDouble());

                    entry.setFirstPartyType(entryObj.value(QStringLiteral("first_party_type")).toString());
                    entry.setSecondPartyType(entryObj.value(QStringLiteral("second_party_type")).toString());

                    if (entryObj.contains(QStringLiteral("extra_info")))
                    {
                        const auto extraInfo = entryObj.value(QStringLiteral("extra_info")).toObject();
                        const auto checkAndSetExtraInfo = [&](const auto &key) {
                            if (extraInfo.contains(key))
                            {
                                entry.setExtraInfoId(extraInfo.value(key).toDouble());
                                entry.setExtraInfoType(key);
                                return true;
                            }

                            return false;
                        };

                        checkAndSetExtraInfo(QStringLiteral("alliance_id")) &&
                        checkAndSetExtraInfo(QStringLiteral("character_id")) &&
                        checkAndSetExtraInfo(QStringLiteral("contract_id")) &&
                        checkAndSetExtraInfo(QStringLiteral("corporation_id")) &&
                        checkAndSetExtraInfo(QStringLiteral("destroyed_ship_type_id")) &&
                        checkAndSetExtraInfo(QStringLiteral("job_id")) &&
                        checkAndSetExtraInfo(QStringLiteral("location_id")) &&
                        checkAndSetExtraInfo(QStringLiteral("npc_id")) &&
                        checkAndSetExtraInfo(QStringLiteral("planet_id")) &&
                        checkAndSetExtraInfo(QStringLiteral("system_id")) &&
                        checkAndSetExtraInfo(QStringLiteral("transaction_id"));
                    }

                    entry.setReason(entryObj.value(QStringLiteral("reason")).toString());

                    if (entryObj.contains(QStringLiteral("amount")))
                        entry.setAmount(entryObj.value(QStringLiteral("amount")).toDouble());
                    if (entryObj.contains(QStringLiteral("balance")))
                        entry.setBalance(entryObj.value(QStringLiteral("balance")).toDouble());
                    if (entryObj.contains(QStringLiteral("tax_reciever_id")))
                        entry.setTaxReceiverId(entryObj.value(QStringLiteral("tax_reciever_id")).toDouble());
                    if (entryObj.contains(QStringLiteral("tax")))
                        entry.setTaxAmount(entryObj.value(QStringLiteral("tax")).toDouble());

                    return entry;
                }},
                [](auto &journal, const auto &entry) {
                    journal.emplace(entry);
                }
            );

            callback(std::move(journal), {}, expires);
        });
    }

    void ESIManager::fetchCharacterWalletTransactions(Character::IdType charId,
                                                      const boost::optional<WalletTransaction::IdType> &fromId,
                                                      WalletTransaction::IdType tillId,
                                                      std::shared_ptr<WalletTransactions> &&transactions,
                                                      const Callback<WalletTransactions> &callback) const
    {
        selectNextInterface().fetchCharacterWalletTransactions(
            charId,
            fromId,
            [=, transactions = std::move(transactions)](auto &&data, const auto &error, const auto &expires) mutable {
                if (Q_UNLIKELY(!error.isEmpty()))
                {
                    callback({}, error, expires);
                    return;
                }

                auto nextFromId = std::numeric_limits<WalletTransaction::IdType>::max();
                std::atomic_bool reachedEnd{transactions->empty()};

                std::mutex resultMutex;

                auto transactionsArray = data.array();
                QtConcurrent::blockingMap(
                    transactionsArray,
                    [&](const auto &value) {
                        const auto transactionObj = value.toObject();

                        WalletTransaction transaction{
                            static_cast<WalletTransaction::IdType>(transactionObj.value(QStringLiteral("transaction_id")).toDouble())
                        };

                        const auto id = transaction.getId();
                        if (id > tillId)
                        {
                            transaction.setCharacterId(charId);
                            transaction.setTimestamp(getDateTimeFromString(transactionObj.value(QStringLiteral("date")).toString()));
                            transaction.setQuantity(transactionObj.value(QStringLiteral("quantity")).toDouble());
                            transaction.setTypeId(transactionObj.value(QStringLiteral("type_id")).toDouble());
                            transaction.setPrice(transactionObj.value(QStringLiteral("unit_price")).toDouble());
                            transaction.setClientId(transactionObj.value(QStringLiteral("client_id")).toDouble());
                            transaction.setLocationId(transactionObj.value(QStringLiteral("location_id")).toDouble());
                            transaction.setType(
                                (transactionObj.value(QStringLiteral("is_buy")).toBool()) ?
                                (WalletTransaction::Type::Buy) :
                                (WalletTransaction::Type::Sell)
                            );
                            transaction.setJournalId(transactionObj.value(QStringLiteral("journal_ref_id")).toDouble());

                            {
                                std::lock_guard<std::mutex> lock{resultMutex};

                                transactions->emplace(std::move(transaction));
                                if (nextFromId > id)
                                    nextFromId = id;
                            }
                        }
                        else
                        {
                            reachedEnd = true;
                        }
                    }
                );

                if (reachedEnd)
                {
                    callback(std::move(*transactions), {}, expires);
                }
                else
                {
                    fetchCharacterWalletTransactions(
                        charId,
                        nextFromId,
                        tillId,
                        std::move(transactions),
                        callback
                    );
                }
        });
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
                    const auto accessToken = object.value(QStringLiteral("access_token")).toString().toLatin1();
                    const auto refreshToken = object.value(QStringLiteral("refresh_token")).toString();

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
                        const Character::IdType realCharId = object["CharacterID"].toDouble(Character::invalidId);

                        mRefreshTokens[realCharId] = refreshToken;

                        QSettings settings;
                        settings.setValue(SSOSettings::refreshTokenKey.arg(realCharId), mCrypt.encryptToByteArray(refreshToken));

                        if (charId != realCharId)
                        {
                            qDebug() << "Logged as invalid character id:" << realCharId;
                            emit tokenError(charId, tr("Please authorize access for character: %1").arg(getCharacterName(charId).first));
                            return;
                        }

                        emit acquiredToken(realCharId, accessToken,
                                           QDateTime::currentDateTime().addSecs(object.value(QStringLiteral("expires_in")).toInt() - 10));
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
            QByteArrayLiteral("Authorization"), QByteArrayLiteral("Basic ") + (mClientId + ":" + mClientSecret).toBase64());

        return request;
    }

    ExternalOrder ESIManager::getExternalOrderFromJson(const QJsonObject &object, uint regionId) const
    {
        const auto range = object.value(QStringLiteral("range")).toString();

        ExternalOrder order;

        order.setId(object.value(QStringLiteral("order_id")).toDouble()); // https://bugreports.qt.io/browse/QTBUG-28560
        order.setType((object.value(QStringLiteral("is_buy_order")).toBool()) ? (ExternalOrder::Type::Buy) : (ExternalOrder::Type::Sell));
        order.setTypeId(object.value(QStringLiteral("type_id")).toDouble());
        order.setStationId(object.value(QStringLiteral("location_id")).toDouble());

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
        order.setPrice(object.value(QStringLiteral("price")).toDouble());
        order.setVolumeEntered(object.value(QStringLiteral("volume_total")).toInt());
        order.setVolumeRemaining(object.value(QStringLiteral("volume_remain")).toInt());
        order.setMinVolume(object.value(QStringLiteral("min_volume")).toInt());
        order.setIssued(getDateTimeFromString(object.value(QStringLiteral("issued")).toString()));
        order.setDuration(object.value(QStringLiteral("duration")).toInt());

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
                (*orders)[nextIndex++] = getExternalOrderFromJson(item.toObject(), regionId);
            };

            QtConcurrent::blockingMap(items, parseItem);

            if (atEnd)
                callback(std::move(*orders), {}, expires);
        };
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
        return mInterfaceManager.selectNextInterface();
    }

    std::pair<QString, bool> ESIManager::getCharacterName(Character::IdType id) const
    {
        QString charName;
        auto nameFound = false;

        try
        {
            charName = mCharacterRepo.getName(id);
            nameFound = true;
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            nameFound = mDataProvider.hasGenericName(id);
            charName = mDataProvider.getGenericName(id);
        }

        return std::make_pair(charName, nameFound);
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

    QDateTime ESIManager::getDateTimeFromString(const QString &value)
    {
        auto dt = QDateTime::fromString(value, Qt::ISODate);
        if (Q_UNLIKELY(!dt.isValid()))
            dt = QDateTime::currentDateTimeUtc();   // just to be safe
        else
            dt.setTimeSpec(Qt::UTC);

        return dt;
    }

    QNetworkRequest ESIManager::getVerifyRequest(const QByteArray &accessToken)
    {
        QNetworkRequest request{loginUrl + "/oauth/verify"};
        request.setRawHeader(QByteArrayLiteral("Authorization"), QByteArrayLiteral("Bearer  ") + accessToken);

        return request;
    }

    QString ESIManager::getAuthWidowTitle(const QString &charName)
    {
        return tr("SSO Authentication for character: %1").arg(charName);
    }
}
