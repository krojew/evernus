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
