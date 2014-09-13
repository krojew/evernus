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
#include "ConquerableStationRepository.h"

namespace Evernus
{
    QString ConquerableStationRepository::getTableName() const
    {
        return "conquerable_stations";
    }

    QString ConquerableStationRepository::getIdColumn() const
    {
        return "id";
    }

    ConquerableStationRepository::EntityPtr ConquerableStationRepository::populate(const QSqlRecord &record) const
    {
        auto station = std::make_shared<ConquerableStation>(record.value("id").value<ConquerableStation::IdType>());
        station->setName(record.value("name").toString());
        station->setSolarSystemId(record.value("solar_system_id").toULongLong());
        station->setNew(false);

        return station;
    }

    void ConquerableStationRepository::create() const
    {
        exec(QString{"CREATE TABLE IF NOT EXISTS %1 ("
            "id INTEGER PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "solar_system_id INTEGER NOT NULL"
        ")"}.arg(getTableName()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_solar_system ON %1(solar_system_id)"}.arg(getTableName()));
    }

    ConquerableStationRepository::EntityList ConquerableStationRepository::fetchForSolarSystem(uint solarSystemId) const
    {
        auto query = prepare(QString{"SELECT * FROM %1 WHERE solar_system_id = ?"}.arg(getTableName()));
        query.bindValue(0, solarSystemId);

        DatabaseUtils::execQuery(query);

        EntityList result;

        const auto size = query.size();
        if (size > 0)
            result.reserve(size);

        while (query.next())
            result.emplace_back(populate(query.record()));

        return result;
    }

    QStringList ConquerableStationRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "name"
            << "solar_system_id";
    }

    void ConquerableStationRepository::bindValues(const ConquerableStation &entity, QSqlQuery &query) const
    {
        if (entity.getId() != ConquerableStation::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":name", entity.getName());
        query.bindValue(":solar_system_id", entity.getSolarSystemId());
    }

    void ConquerableStationRepository::bindPositionalValues(const ConquerableStation &entity, QSqlQuery &query) const
    {
        if (entity.getId() != ConquerableStation::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getName());
        query.addBindValue(entity.getSolarSystemId());
    }
}
