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

    ConquerableStation ConquerableStationRepository::populate(const QSqlRecord &record) const
    {
        ConquerableStation station;
        station.setId(record.value("id").value<ConquerableStation::IdType>());
        station.setName(record.value("name").toString());
        station.setNew(false);

        return station;
    }

    void ConquerableStationRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL
        ))"}.arg(getTableName()));
    }

    QStringList ConquerableStationRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "name";
    }

    void ConquerableStationRepository::bindValues(const ConquerableStation &entity, QSqlQuery &query) const
    {
        if (entity.getId() != ConquerableStation::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":name", entity.getName());
    }

    void ConquerableStationRepository::bindPositionalValues(const ConquerableStation &entity, QSqlQuery &query) const
    {
        if (entity.getId() != ConquerableStation::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getName());
    }
}
