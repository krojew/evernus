#include "CachedCharacterListRepository.h"

namespace Evernus
{
    QString CachedCharacterListRepository::getTableName() const
    {
        return "character_lists";
    }

    QString CachedCharacterListRepository::getIdColumn() const
    {
        return "key_id";
    }

    CachedCharacterList CachedCharacterListRepository::populate(const QSqlRecord &record) const
    {
        const auto data = record.value("data").toByteArray();
        CachedCharacterList::CharacterList listData(data.size() / sizeof(CachedCharacterList::CharacterList::value_type));

        std::memcpy(listData.data(), data.constData(), data.size());

        auto cacheUntil = record.value("cache_until").toDateTime();
        cacheUntil.setTimeSpec(Qt::UTC);

        CachedCharacterList list;
        list.setId(record.value("key_id").value<CachedCharacterList::IdType>());
        list.setCacheUntil(cacheUntil);
        list.setCharacterList(std::move(listData));
        list.setNew(false);

        return list;
    }

    void CachedCharacterListRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            key_id INTEGER PRIMARY KEY,
            cache_until TEXT NOT NULL,
            data BLOB NOT NULL
        ))"}.arg(getTableName()));
    }

    QStringList CachedCharacterListRepository::getColumns() const
    {
        return QStringList{}
            << "key_id"
            << "cache_until"
            << "data";
    }

    void CachedCharacterListRepository::bindValues(const CachedCharacterList &entity, QSqlQuery &query) const
    {
        const auto data = entity.getCharacterList();

        query.bindValue(":key_id", entity.getId());
        query.bindValue(":cache_until", entity.getCacheUntil());
        query.bindValue(":data", QByteArray{
            reinterpret_cast<const char *>(data.data()),
            static_cast<int>(data.size() * sizeof(CachedCharacterList::CharacterList::value_type))});
    }
}
