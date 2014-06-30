#include "CachedConquerableStationListRepository.h"

namespace Evernus
{
    QString CachedConquerableStationListRepository::getTableName() const
    {
        return "conquerable_station_lists";
    }

    QString CachedConquerableStationListRepository::getIdColumn() const
    {
        return "id";
    }

    CachedConquerableStationList CachedConquerableStationListRepository::populate(const QSqlRecord &record) const
    {
        auto cacheUntil = record.value("cache_until").toDateTime();
        cacheUntil.setTimeSpec(Qt::UTC);

        CachedConquerableStationList list;
        list.setId(record.value("id").value<CachedConquerableStationList::IdType>());
        list.setCacheUntil(cacheUntil);
        list.setNew(false);

        return list;
    }

    void CachedConquerableStationListRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY ASC,
            cache_until TEXT NOT NULL
        ))"}.arg(getTableName()));
    }

    QStringList CachedConquerableStationListRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "cache_until";
    }

    void CachedConquerableStationListRepository::bindValues(const CachedConquerableStationList &entity, QSqlQuery &query) const
    {
        if (!entity.isNew())
            query.bindValue(":id", entity.getId());

        query.bindValue(":cache_until", entity.getCacheUntil());
    }
}
