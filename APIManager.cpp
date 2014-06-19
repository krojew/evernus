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
