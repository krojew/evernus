#include <QXmlQuery>
#include <QDateTime>

#include "APIManager.h"

namespace Evernus
{
    const QString APIManager::eveTimeFormat = "yyyy-MM-dd HH:mm:ss";

    void APIManager::fetchCharacterList(const Key &key, const std::function<void (const CharacterList &)> &callback)
    {
        if (mCache.hasChracterListData(key.getId()))
        {
            callback(mCache.getCharacterListData(key.getId()));
            return;
        }

        mInterface.fetchCharacterList(key, [callback](const QString &response) {
            const auto currentTime = QDateTime::fromString(queryPath("/eveapi/currentTime/text()", response), eveTimeFormat);
            const auto cachedUntil = QDateTime::fromString(queryPath("/eveapi/cachedUntil/text()", response), eveTimeFormat);

            const std::chrono::seconds cacheDelta{currentTime.secsTo(cachedUntil)};
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
