#include "CachedConquerableStationListRepository.h"

#include "CachedConquerableStationRepository.h"

namespace Evernus
{
    QString CachedConquerableStationRepository::getTableName() const
    {
        return "conquerable_stations";
    }

    QString CachedConquerableStationRepository::getIdColumn() const
    {
        return "id";
    }

    CachedConquerableStation CachedConquerableStationRepository::populate(const QSqlRecord &record) const
    {
        CachedConquerableStation station;
        station.setId(record.value("id").value<CachedConquerableStation::IdType>());
        station.setListId(record.value("list_id").value<CachedConquerableStationList::IdType>());
        station.setName(record.value("name").toString());
        station.setNew(false);

        return station;
    }

    void CachedConquerableStationRepository::create(const CachedConquerableStationListRepository &listRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY ASC,
            list_id INTEGER NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE,
            name TEXT NOT NULL
        ))"}.arg(getTableName()).arg(listRepo.getTableName()).arg(listRepo.getIdColumn()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_%2_index ON %1(list_id)"}.arg(getTableName()).arg(listRepo.getTableName()));
    }

    QStringList CachedConquerableStationRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "list_id"
            << "name";
    }

    void CachedConquerableStationRepository::bindValues(const CachedConquerableStation &entity, QSqlQuery &query) const
    {
        query.bindValue(":id", entity.getId());
        query.bindValue(":list_id", entity.getListId());
        query.bindValue(":name", entity.getName());
    }
}
