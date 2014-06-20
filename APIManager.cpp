#include <QXmlQuery>
#include <QDateTime>

#include "APIManager.h"

namespace Evernus
{
    const QString APIManager::eveTimeFormat = "yyyy-MM-dd HH:mm:ss";

    APIManager::APIManager()
        : QObject{}
    {
        connect(&mInterface, &APIInterface::error, this, &APIManager::error);
    }

    void APIManager::fetchCharacterList(const Key &key, const Callback<CharacterList> &callback)
    {
        if (mCache.hasChracterListData(key.getId()))
        {
            callback(mCache.getCharacterListData(key.getId()), true);
            return;
        }

        mInterface.fetchCharacterList(key, [key, callback, this](const QString &response, bool success) {
            auto cachedUntil = QDateTime::fromString(queryPath("/eveapi/cachedUntil/text()", response), eveTimeFormat);
            cachedUntil.setTimeSpec(Qt::UTC);
        });
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
