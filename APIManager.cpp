#include <QXmlQuery>
#include <QDateTime>

#include "APIManager.h"

namespace Evernus
{
    const QString APIManager::eveTimeFormat = "yyyy-MM-dd HH:mm:ss";

    APIManager::APIManager()
        : QObject{}
    {
        connect(&mInterface, &APIInterface::generalError, this, &APIManager::generalError);
    }

    void APIManager::fetchCharacterList(const Key &key, const Callback<CharacterList> &callback)
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

                auto cachedUntil = QDateTime::fromString(queryPath("/eveapi/cachedUntil/text()", response), eveTimeFormat);
                cachedUntil.setTimeSpec(Qt::UTC);

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

    void APIManager::handlePotentialError(const QString &xml, const QString &error)
    {
        if (!error.isEmpty())
            throw std::runtime_error{error.toStdString()};

        if (xml.isEmpty())
            throw std::runtime_error{tr("No XML document received!").toUtf8().constData()};

        const auto errorText = queryPath("/eveapi/error/text()", xml);
        if (!errorText.isEmpty())
            throw std::runtime_error{errorText.toStdString()};
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

    QString APIManager::queryPath(const QString &path, const QString &xml)
    {
        QString out;

        QXmlQuery query;
        query.setFocus(xml);
        query.setQuery(path);
        query.evaluateTo(&out);

        return out.trimmed();
    }
}
