#include <QDomDocument>
#include <QXmlQuery>
#include <QDateTime>

#include "CharacterListXmlReceiver.h"
#include "CharacterDomParser.h"

#include "APIManager.h"

namespace Evernus
{
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

        mInterface.fetchCharacterList(key, [key, callback, this](const QString &response, const QString &error) {
            try
            {
                handlePotentialError(response, error);

                auto cachedUntil = getCachedUntil(response);

                CharacterList list{parseResults<Character::IdType>(response, "characters")};
                mCache.setChracterListData(key.getId(), list, cachedUntil);

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
        if (mCache.hasCharacterData(key.getId(), characterId))
        {
            callback(mCache.getCharacterData(key.getId(), characterId), QString{});
            return;
        }

        mInterface.fetchCharacter(key, characterId, [key, callback, this](const QString &response, const QString &error) {
            try
            {
                handlePotentialError(response, error);

                auto cachedUntil = getCachedUntil(response);

                Character character{parseResult<Character>(response)};
                character.setKeyId(key.getId());
                mCache.setCharacterData(key.getId(), character.getId(), character, cachedUntil);

                callback(character, QString{});
            }
            catch (const std::exception &e)
            {
                callback(Character{}, e.what());
            }
        });
    }

    template<class T>
    std::vector<T> APIManager::parseResults(const QString &xml, const QString &rowsetName)
    {
        std::vector<T> result;

        QXmlQuery query;
        query.setFocus(xml);
        query.setQuery(QString{"//rowset[@name='%1']/row"}.arg(rowsetName));

        APIXmlReceiver<T> receiver{result, query.namePool()};
        query.evaluateTo(&receiver);

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
