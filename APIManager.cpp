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
#include <thread>

#include <QAbstractMessageHandler>
#include <QDomDocument>
#include <QXmlQuery>
#include <QDateTime>
#include <QHash>

#include "ConquerableStationListXmlReceiver.h"
#include "WalletTransactionsXmlReceiver.h"
#include "WalletJournalXmlReceiver.h"
#include "CharacterListXmlReceiver.h"
#include "ContractItemsXmlReceiver.h"
#include "MarketOrdersXmlReceiver.h"
#include "ContractsXmlReceiver.h"
#include "AssetListXmlReceiver.h"
#include "GenericNameDomParser.h"
#include "RefTypeXmlReceiver.h"
#include "CharacterDomParser.h"
#include "CacheTimerProvider.h"
#include "APIUtils.h"
#include "CorpKey.h"
#include "Defines.h"
#include "Item.h"

#include "APIManager.h"

namespace Evernus
{
    namespace
    {
        class APIXmlMessageHandler
            : public QAbstractMessageHandler
        {
        public:
            explicit APIXmlMessageHandler(QString &error)
                : QAbstractMessageHandler{}
                , mError{error}
            {
            }

            virtual ~APIXmlMessageHandler() = default;

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

    APIManager::APIManager(CacheTimerProvider &cacheTimerProvider)
        : QObject{}
        , mCacheTimerProvider{cacheTimerProvider}
    {
    }

    void APIManager::fetchCharacterList(const Key &key, const Callback<CharacterList> &callback) const
    {
        if (mPendingCharacterListRequests.find(key.getId()) != std::end(mPendingCharacterListRequests))
            return;

        mPendingCharacterListRequests.emplace(key.getId());

        mInterface.fetchCharacterList(key, [=](const QString &response, const QString &error) {
            mPendingCharacterListRequests.erase(key.getId());

            try
            {
                handlePotentialError(response, error);

                CharacterList list{parseResults<Character::IdType, APIXmlReceiver<Character::IdType>::CurElemType>(response, "characters")};
                callback(std::move(list), QString{});
            }
            catch (const std::exception &e)
            {
                callback(CharacterList{}, e.what());
            }
        });
    }

    void APIManager::fetchCharacter(const Key &key, Character::IdType characterId, const Callback<Character> &callback) const
    {
        if (mPendingCharacterRequests.find(characterId) != std::end(mPendingCharacterRequests))
            return;

        mPendingCharacterRequests.emplace(characterId);

        mInterface.fetchCharacter(key, characterId, [=](const QString &response, const QString &error) {
            mPendingCharacterRequests.erase(characterId);

            try
            {
                handlePotentialError(response, error);

                Character character{parseResult<Character>(response)};
                character.setKeyId(key.getId());

                mCacheTimerProvider.setUtcCacheTimer(characterId,
                                                     TimerType::Character,
                                                     APIUtils::getCachedUntil(response));

                callback(std::move(character), QString{});
            }
            catch (const std::exception &e)
            {
                callback(Character{}, e.what());
            }
        });
    }

    void APIManager::fetchAssets(const Key &key, Character::IdType characterId, const Callback<AssetList> &callback) const
    {
        if (mPendingAssetsRequests.find(characterId) != std::end(mPendingAssetsRequests))
            return;

        mPendingAssetsRequests.emplace(characterId);

        mInterface.fetchAssets(key, characterId, [=](const QString &response, const QString &error) {
            mPendingAssetsRequests.erase(characterId);

            try
            {
                handlePotentialError(response, error);

                AssetList assets{parseResults<AssetList::ItemType, std::unique_ptr<AssetList::ItemType::element_type>>(response, "assets")};
                assets.setCharacterId(characterId);

                mCacheTimerProvider.setUtcCacheTimer(characterId,
                                                     TimerType::AssetList,
                                                     APIUtils::getCachedUntil(response));

                callback(std::move(assets), QString{});
            }
            catch (const std::exception &e)
            {
                callback(AssetList{}, e.what());
            }
        });
    }

    void APIManager::fetchAssets(const CorpKey &key, Character::IdType characterId, const Callback<AssetList> &callback) const
    {
        mInterface.fetchAssets(key, [=](const QString &response, const QString &error) {
            try
            {
                handlePotentialError(response, error);

                AssetList assets{parseResults<AssetList::ItemType, std::unique_ptr<AssetList::ItemType::element_type>>(response, "assets")};
                assets.setCharacterId(characterId);

                mCacheTimerProvider.setUtcCacheTimer(characterId,
                                                     TimerType::CorpAssetList,
                                                     APIUtils::getCachedUntil(response));

                callback(std::move(assets), QString{});
            }
            catch (const std::exception &e)
            {
                callback(AssetList{}, e.what());
            }
        });
    }

    void APIManager::fetchConquerableStationList(const Callback<ConquerableStationList> &callback) const
    {
#if EVERNUS_CLANG_LAMBDA_CAPTURE_BUG
        mInterface.fetchConquerableStationList([=, callback = callback](const QString &response, const QString &error) {
#else
        mInterface.fetchConquerableStationList([=](const QString &response, const QString &error) {
#endif
            try
            {
                handlePotentialError(response, error);

                ConquerableStationList stations{parseResults<ConquerableStation, APIXmlReceiver<ConquerableStation>::CurElemType>(response, "outposts")};
                callback(std::move(stations), QString{});
            }
            catch (const std::exception &e)
            {
                callback(ConquerableStationList{}, e.what());
            }
        });
    }

    void APIManager::fetchRefTypes(const Callback<RefTypeList> &callback) const
    {
#if EVERNUS_CLANG_LAMBDA_CAPTURE_BUG
        mInterface.fetchRefTypes([=, callback = callback](const QString &response, const QString &error) {
#else
        mInterface.fetchRefTypes([=](const QString &response, const QString &error) {
#endif
            try
            {
                handlePotentialError(response, error);

                callback(parseResults<RefType, APIXmlReceiver<RefType>::CurElemType>(response, "refTypes"), QString{});
            }
            catch (const std::exception &e)
            {
                callback(RefTypeList{}, e.what());
            }
        });
    }

    void APIManager::fetchWalletJournal(const Key &key,
                                        Character::IdType characterId,
                                        WalletJournalEntry::IdType fromId,
                                        WalletJournalEntry::IdType tillId,
                                        const Callback<WalletJournal> &callback) const
    {
        fetchWalletJournal(key,
                           characterId,
                           0,
                           fromId,
                           tillId,
                           defaultWalletAccountKey,
                           std::make_shared<WalletJournal>(),
                           callback,
                           "transactions",
                           TimerType::WalletJournal,
                           false);
    }

    void APIManager::fetchWalletJournal(const CorpKey &key,
                                        Character::IdType characterId,
                                        quint64 corpId,
                                        WalletJournalEntry::IdType fromId,
                                        WalletJournalEntry::IdType tillId,
                                        int accountKey,
                                        const Callback<WalletJournal> &callback) const
    {
        fetchWalletJournal(key,
                           characterId,
                           corpId,
                           fromId,
                           tillId,
                           accountKey,
                           std::make_shared<WalletJournal>(),
                           callback,
                           "entries",
                           TimerType::CorpWalletJournal,
                           false);
    }

    void APIManager::fetchWalletTransactions(const Key &key,
                                             Character::IdType characterId,
                                             WalletTransaction::IdType fromId,
                                             WalletTransaction::IdType tillId,
                                             const Callback<WalletTransactions> &callback) const
    {
        fetchWalletTransactions(key,
                                characterId,
                                0,
                                fromId,
                                tillId,
                                defaultWalletAccountKey,
                                std::make_shared<WalletTransactions>(),
                                callback,
                                TimerType::WalletTransactions,
                                false);
    }

    void APIManager::fetchWalletTransactions(const CorpKey &key,
                                             Character::IdType characterId,
                                             quint64 corpId,
                                             WalletTransaction::IdType fromId,
                                             WalletTransaction::IdType tillId,
                                             int accountKey,
                                             const Callback<WalletTransactions> &callback) const
    {
        fetchWalletTransactions(key,
                                characterId,
                                corpId,
                                fromId,
                                tillId,
                                accountKey,
                                std::make_shared<WalletTransactions>(),
                                callback,
                                TimerType::CorpWalletTransactions,
                                false);
    }

    void APIManager::fetchMarketOrders(const Key &key, Character::IdType characterId, const Callback<MarketOrders> &callback) const
    {
        doFetchMarketOrders(key, characterId, callback, TimerType::MarketOrders);
    }

    void APIManager::fetchMarketOrders(const CorpKey &key, Character::IdType characterId, const Callback<MarketOrders> &callback) const
    {
        doFetchMarketOrders(key, characterId, callback, TimerType::CorpMarketOrders);
    }

    void APIManager::fetchContracts(const Key &key, Character::IdType characterId, const Callback<Contracts> &callback) const
    {
        doFetchContracts(key, characterId, callback, TimerType::Contracts);
    }

    void APIManager::fetchContracts(const CorpKey &key, Character::IdType characterId, const Callback<Contracts> &callback) const
    {
        doFetchContracts(key, characterId, callback, TimerType::CorpContracts);
    }

    void APIManager::fetchContractItems(const Key &key,
                                        Character::IdType characterId,
                                        Contract::IdType contractId,
                                        const Callback<ContractItemList> &callback) const
    {
        doFetchContractItems(key, characterId, contractId, callback);
    }

    void APIManager::fetchContractItems(const CorpKey &key,
                                        Character::IdType characterId,
                                        Contract::IdType contractId,
                                        const Callback<ContractItemList> &callback) const
    {
        doFetchContractItems(key, characterId, contractId, callback);
    }

    void APIManager::fetchGenericName(quint64 id, const Callback<QString> &callback) const
    {
#if EVERNUS_CLANG_LAMBDA_CAPTURE_BUG
        mInterface.fetchGenericName(id, [=, callback = callback](const QString &response, const QString &error) {
#else
        mInterface.fetchGenericName(id, [=](const QString &response, const QString &error) {
#endif
            try
            {
                handlePotentialError(response, error);

                callback(parseResult<QString>(response), QString{});
            }
            catch (const std::exception &e)
            {
                callback(QString{}, e.what());
            }
        });
    }

    template<class Key>
    void APIManager::fetchWalletJournal(const Key &key,
                                        Character::IdType characterId,
                                        quint64 corpId,
                                        WalletJournalEntry::IdType fromId,
                                        WalletJournalEntry::IdType tillId,
                                        int accountKey,
                                        std::shared_ptr<WalletJournal> &&journal,
                                        const Callback<WalletJournal> &callback,
                                        const QString &rowsetName,
                                        TimerType timerType,
                                        bool retry) const
    {
        mInterface.fetchWalletJournal(key, characterId, fromId, accountKey,
                                      [=](const QString &response, const QString &error) mutable {
            try
            {
                handlePotentialError(response, error);

                auto parsed
                    = parseResults<WalletJournal::value_type, APIXmlReceiver<WalletJournal::value_type>::CurElemType>(response, rowsetName);

                auto reachedEnd = parsed.empty();
                auto nextFromId = std::numeric_limits<WalletJournalEntry::IdType>::max();

                for (auto &entry : parsed)
                {
                    const auto id = entry.getId();
                    if (id > tillId)
                    {
                        entry.setCharacterId(characterId);
                        entry.setCorporationId(corpId);
                        journal->emplace(std::move(entry));

                        if (nextFromId > id)
                            nextFromId = id;
                    }
                    else
                    {
                        reachedEnd = true;
                    }
                }

                if (reachedEnd)
                {
                    mCacheTimerProvider.setUtcCacheTimer(characterId,
                                                         timerType,
                                                         APIUtils::getCachedUntil(response));

                    callback(std::move(*journal), QString{});
                }
                else
                {
                    fetchWalletJournal(key,
                                       characterId,
                                       corpId,
                                       nextFromId,
                                       tillId,
                                       accountKey,
                                       std::move(journal),
                                       callback,
                                       rowsetName,
                                       timerType,
                                       false);
                }
            }
            catch (const std::exception &e)
            {
                if (retry)
                {
                    callback(WalletJournal{}, e.what());
                }
                else
                {
                    qDebug() << "Retrying fetching wallet journal...";

                    // EVE API bug workaround
                    std::this_thread::sleep_for(std::chrono::seconds{1});
                    fetchWalletJournal(key,
                                       characterId,
                                       corpId,
                                       fromId,
                                       tillId,
                                       accountKey,
                                       std::move(journal),
                                       callback,
                                       rowsetName,
                                       timerType,
                                       true);
                }
            }
        });
    }

    template<class Key>
    void APIManager::fetchWalletTransactions(const Key &key,
                                             Character::IdType characterId,
                                             quint64 corpId,
                                             WalletTransaction::IdType fromId,
                                             WalletTransaction::IdType tillId,
                                             int accountKey,
                                             std::shared_ptr<WalletTransactions> &&transactions,
                                             const Callback<WalletTransactions> &callback,
                                             TimerType timerType,
                                             bool retry) const
    {
        mInterface.fetchWalletTransactions(key, characterId, fromId, accountKey,
                                           [=](const QString &response, const QString &error) mutable {
            try
            {
                handlePotentialError(response, error);

                auto parsed
                    = parseResults<WalletTransactions::value_type, APIXmlReceiver<WalletTransactions::value_type>::CurElemType>(response, "transactions");

                auto reachedEnd = parsed.empty();
                auto nextFromId = std::numeric_limits<WalletTransaction::IdType>::max();

                for (auto &entry : parsed)
                {
                    const auto id = entry.getId();
                    if (id > tillId)
                    {
                        entry.setCharacterId(characterId);
                        entry.setCorporationId(corpId);
                        transactions->emplace(std::move(entry));

                        if (nextFromId > id)
                            nextFromId = id;
                    }
                    else
                    {
                        reachedEnd = true;
                    }
                }

                if (reachedEnd)
                {
                    mCacheTimerProvider.setUtcCacheTimer(characterId,
                                                         timerType,
                                                         APIUtils::getCachedUntil(response));

                    callback(std::move(*transactions), QString{});
                }
                else
                {
                    fetchWalletTransactions(key,
                                            characterId,
                                            corpId,
                                            nextFromId,
                                            tillId,
                                            accountKey,
                                            std::move(transactions),
                                            callback,
                                            timerType,
                                            false);
                }
            }
            catch (const std::exception &e)
            {
                if (retry)
                {
                    callback(WalletTransactions{}, e.what());
                }
                else
                {
                    qDebug() << "Retrying fetching wallet transactions...";

                    // EVE API bug workaround
                    std::this_thread::sleep_for(std::chrono::seconds{1});
                    fetchWalletTransactions(key,
                                            characterId,
                                            corpId,
                                            fromId,
                                            tillId,
                                            accountKey,
                                            std::move(transactions),
                                            callback,
                                            timerType,
                                            true);
                }
            }
        });
    }

    template<class Key>
    void APIManager::doFetchMarketOrders(const Key &key, Character::IdType characterId, const Callback<MarketOrders> &callback, TimerType timerType) const
    {
#if EVERNUS_CLANG_LAMBDA_CAPTURE_BUG
        mInterface.fetchMarketOrders(key, characterId, [=, callback = callback](const QString &response, const QString &error) {
#else
        mInterface.fetchMarketOrders(key, characterId, [=](const QString &response, const QString &error) {
#endif
            try
            {
                handlePotentialError(response, error);

                mCacheTimerProvider.setUtcCacheTimer(characterId,
                                                     timerType,
                                                     APIUtils::getCachedUntil(response));

                callback(parseResults<MarketOrders::value_type, APIXmlReceiver<MarketOrders::value_type>::CurElemType>(response, "orders"), QString{});
            }
            catch (const std::exception &e)
            {
                callback(MarketOrders{}, e.what());
            }
        });
    }

    template<class Key>
    void APIManager::doFetchContracts(const Key &key, Character::IdType characterId, const Callback<Contracts> &callback, TimerType timerType) const
    {
        mInterface.fetchContracts(key, characterId, [=](const QString &response, const QString &error) {
            try
            {
                handlePotentialError(response, error);

                mCacheTimerProvider.setUtcCacheTimer(characterId,
                                                     timerType,
                                                     APIUtils::getCachedUntil(response));

                callback(parseResults<Contracts::value_type, APIXmlReceiver<Contracts::value_type>::CurElemType>(response, "contractList"), QString{});
            }
            catch (const std::exception &e)
            {
                callback(Contracts{}, e.what());
            }
        });
    }

    template<class Key>
    void APIManager::doFetchContractItems(const Key &key,
                                          Character::IdType characterId,
                                          Contract::IdType contractId,
                                          const Callback<ContractItemList> &callback) const
    {
        mInterface.fetchContractItems(key, characterId, contractId, [=](const QString &response, const QString &error) {
            try
            {
                handlePotentialError(response, error);

                auto result = parseResults<ContractItem, APIXmlReceiver<ContractItem>::CurElemType>(response, "itemList");
                for (auto &item : result)
                    item.setContractId(contractId);

                callback(std::move(result), QString{});
            }
            catch (const std::exception &e)
            {
                callback(ContractItemList{}, e.what());
            }
        });
    }

    template<class T, class CurElem>
    std::vector<T> APIManager::parseResults(const QString &xml, const QString &rowsetName)
    {
        std::vector<T> result;
        QString error;

        APIXmlMessageHandler handler{error};

        QXmlQuery query;
        query.setMessageHandler(&handler);
        query.setFocus(xml);
        query.setQuery(QString{"//rowset[@name='%1']/row"}.arg(rowsetName));

        APIXmlReceiver<T, CurElem> receiver{result, query.namePool()};
        query.evaluateTo(&receiver);

        if (!error.isEmpty())
            throw std::runtime_error{error.toStdString()};

        return result;
    }

    template<class T>
    T APIManager::parseResult(const QString &xml)
    {
        QDomDocument document;
        if (!document.setContent(xml))
            throw std::runtime_error{tr("Invalid XML document received!").toStdString()};

        return APIDomParser::parse<T>(document.documentElement().firstChildElement("result"));
    }

    QString APIManager::queryPath(const QString &path, const QString &xml)
    {
        QString out;

        QXmlQuery query;
        query.setFocus(xml);
        query.setQuery(path);
        query.evaluateTo(&out);

        return out.trimmed();
    }

    void APIManager::handlePotentialError(const QString &xml, const QString &error)
    {
        if (!error.isEmpty())
            throw std::runtime_error{error.toStdString()};

        if (xml.isEmpty())
            throw std::runtime_error{tr("No XML document received!").toStdString()};

        const auto errorText = queryPath("/eveapi/error/text()", xml);
        if (!errorText.isEmpty())
            throw std::runtime_error{errorText.toStdString()};
    }
}
