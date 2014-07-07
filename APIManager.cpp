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
#include <QDomDocument>
#include <QXmlQuery>
#include <QDateTime>

#include "ConquerableStationListXmlReceiver.h"
#include "WalletJournalXmlReceiver.h"
#include "CharacterListXmlReceiver.h"
#include "AssetListXmlReceiver.h"
#include "RefTypeXmlReceiver.h"
#include "CharacterDomParser.h"
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

    const QString APIManager::eveTimeFormat = "yyyy-MM-dd HH:mm:ss";

    APIManager::APIManager()
        : QObject{}
    {
        connect(&mInterface, &APIInterface::generalError, this, &APIManager::generalError);
    }

    void APIManager::fetchCharacterList(const Key &key, const Callback<CharacterList> &callback) const
    {
        if (mCache.hasChracterListData(key.getId()))
        {
            callback(mCache.getCharacterListData(key.getId()), QString{});
            return;
        }

        if (mPendingCharacterListRequests.find(key.getId()) != std::end(mPendingCharacterListRequests))
            return;

        mPendingCharacterListRequests.emplace(key.getId());

        mInterface.fetchCharacterList(key, [key, callback, this](const QString &response, const QString &error) {
            mPendingCharacterListRequests.erase(key.getId());

            try
            {
                handlePotentialError(response, error);

                CharacterList list{parseResults<Character::IdType, APIXmlReceiver<Character::IdType>::CurElemType>(response, "characters")};
                mCache.setChracterListData(key.getId(), list, getCachedUntil(response));

                callback(list, QString{});
            }
            catch (const std::exception &e)
            {
                callback(CharacterList{}, e.what());
            }
        });
    }

    void APIManager::fetchCharacter(const Key &key, Character::IdType characterId, const Callback<Character> &callback) const
    {
        if (mCache.hasCharacterData(characterId))
        {
            auto data = mCache.getCharacterData(characterId);
            data.setKeyId(key.getId());

            callback(std::move(data), QString{});
            return;
        }

        if (mPendingCharacterRequests.find(characterId) != std::end(mPendingCharacterRequests))
            return;

        mPendingCharacterRequests.emplace(characterId);

        mInterface.fetchCharacter(key, characterId, [key, callback, characterId, this](const QString &response, const QString &error) {
            mPendingCharacterRequests.erase(characterId);

            try
            {
                handlePotentialError(response, error);

                Character character{parseResult<Character>(response)};
                character.setKeyId(key.getId());
                mCache.setCharacterData(character.getId(), character, getCachedUntil(response));

                callback(character, QString{});
            }
            catch (const std::exception &e)
            {
                callback(Character{}, e.what());
            }
        });
    }

    void APIManager::fetchAssets(const Key &key, Character::IdType characterId, const Callback<AssetList> &callback) const
    {
        if (mCache.hasAssetData(characterId))
        {
            callback(mCache.getAssetData(characterId), QString{});
            return;
        }

        if (mPendingAssetsRequests.find(characterId) != std::end(mPendingAssetsRequests))
            return;

        mPendingAssetsRequests.emplace(characterId);

        mInterface.fetchAssets(key, characterId, [key, characterId, callback, this](const QString &response, const QString &error) {
            mPendingAssetsRequests.erase(characterId);

            try
            {
                handlePotentialError(response, error);

                AssetList assets{parseResults<AssetList::ItemType, std::unique_ptr<AssetList::ItemType::element_type>>(response, "assets")};
                assets.setCharacterId(characterId);
                mCache.setAssetData(characterId, assets, getCachedUntil(response));

                callback(assets, QString{});
            }
            catch (const std::exception &e)
            {
                callback(AssetList{}, e.what());
            }
        });
    }

    void APIManager::fetchConquerableStationList(const Callback<ConquerableStationList> &callback) const
    {
        if (mCache.hasConquerableStationListData())
        {
            callback(mCache.getConquerableStationListData(), QString{});
            return;
        }

        mInterface.fetchConquerableStationList([callback, this](const QString &response, const QString &error) {
            try
            {
                handlePotentialError(response, error);

                ConquerableStationList stations{parseResults<ConquerableStation, APIXmlReceiver<ConquerableStation>::CurElemType>(response, "outposts")};
                mCache.setConquerableStationListData(stations, getCachedUntil(response));

                callback(stations, QString{});
            }
            catch (const std::exception &e)
            {
                callback(ConquerableStationList{}, e.what());
            }
        });
    }

    void APIManager::fetchRefTypes(const Callback<RefTypeList> &callback) const
    {
#ifdef Q_OS_WIN
        mInterface.fetchRefTypes([callback, this](const QString &response, const QString &error) {
#else
        mInterface.fetchRefTypes([callback = callback, this](const QString &response, const QString &error) {
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
        if (mCache.hasWalletJournalData(characterId))
        {
            auto data = mCache.getWalletJournalData(characterId);
            const auto end = std::end(data);
            auto it = data.find(tillId);

            if (it == end)
                it = data.upper_bound(tillId);

            if (it != end)
                data.erase(it, end);

            callback(data, QString{});
            return;
        }

        fetchWalletJournal(key, characterId, fromId, tillId, std::make_shared<WalletJournal>(), callback);
    }

    QDateTime APIManager::getCharacterLocalCacheTime(Character::IdType characterId) const
    {
        return mCache.getCharacterDataLocalCacheTime(characterId);
    }

    QDateTime APIManager::getAssetsLocalCacheTime(Character::IdType characterId) const
    {
        return mCache.getAssetsDataLocalCacheTime(characterId);
    }

    QDateTime APIManager::getWalletJournalLocalCacheTime(Character::IdType characterId) const
    {
        return mCache.getWalletJournalLocalCacheTime(characterId);
    }

    void APIManager::fetchWalletJournal(const Key &key,
                                        Character::IdType characterId,
                                        WalletJournalEntry::IdType fromId,
                                        WalletJournalEntry::IdType tillId,
                                        std::shared_ptr<WalletJournal> &&journal,
                                        const Callback<WalletJournal> &callback) const
    {
        mInterface.fetchWalletJournal(key, characterId, fromId,
                                      [=, journal = std::move(journal)](const QString &response, const QString &error) mutable {
            try
            {
                handlePotentialError(response, error);

                auto parsed
                    = parseResults<WalletJournal::value_type, APIXmlReceiver<WalletJournal::value_type>::CurElemType>(response, "transactions");

                auto reachedEnd = parsed.empty();
                auto nextFromId = std::numeric_limits<WalletJournalEntry::IdType>::max();

                for (auto &entry : parsed)
                {
                    const auto id = entry.getId();
                    if (id > tillId)
                    {
                        entry.setCharacterId(characterId);
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
                    callback(*journal, QString{});
                }
                else
                {
                    fetchWalletJournal(key, characterId, nextFromId, tillId, std::move(journal), callback);
                }
            }
            catch (const std::exception &e)
            {
                callback(WalletJournal{}, e.what());
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

    QDateTime APIManager::getCachedUntil(const QString &xml)
    {
        auto cachedUntil = QDateTime::fromString(queryPath("/eveapi/cachedUntil/text()", xml), eveTimeFormat);
        cachedUntil.setTimeSpec(Qt::UTC);

        return cachedUntil;
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
