#include "CachedAssetListRepository.h"

namespace Evernus
{
    QString CachedAssetListRepository::getTableName() const
    {
        return "asset_lists";
    }

    QString CachedAssetListRepository::getIdColumn() const
    {
        return "id";
    }

    CachedAssetList CachedAssetListRepository::populate(const QSqlRecord &record) const
    {
        auto cacheUntil = record.value("cache_until").toDateTime();
        cacheUntil.setTimeSpec(Qt::UTC);

        CachedAssetList list;
        list.setId(record.value("id").value<CachedAssetList::IdType>());
        list.setCharacterId(record.value("character_id").value<Character::IdType>());
        list.setCacheUntil(cacheUntil);
        list.setNew(false);

        return list;
    }

    void CachedAssetListRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY ASC,
            character_id BIGINT NOT NULL UNIQUE,
            cache_until TEXT NOT NULL
        ))"}.arg(getTableName()));
    }

    QStringList CachedAssetListRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "character_id"
            << "cache_until";
    }

    void CachedAssetListRepository::bindValues(const CachedAssetList &entity, QSqlQuery &query) const
    {
        if (!entity.isNew())
            query.bindValue(":id", entity.getId());

        query.bindValue(":character_id", entity.getCharacterId());
        query.bindValue(":cache_until", entity.getCacheUntil());
    }
}
