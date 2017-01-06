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
#include "CitadelRepository.h"

namespace Evernus
{
    QString CitadelRepository::getTableName() const
    {
        return "citadels";
    }

    QString CitadelRepository::getIdColumn() const
    {
        return "id";
    }

    CitadelRepository::EntityPtr CitadelRepository::populate(const QSqlRecord &record) const
    {
        auto firstSeen = record.value("first_seen").toDateTime();
        firstSeen.setTimeSpec(Qt::UTC);

        auto lastSeen = record.value("last_seen").toDateTime();
        lastSeen.setTimeSpec(Qt::UTC);

        auto citadel = std::make_shared<Citadel>(record.value("id").value<Citadel::IdType>());
        citadel->setName(record.value("name").toString());
        citadel->setSolarSystemId(record.value("solar_system_id").toULongLong());
        citadel->setX(record.value("x").toDouble());
        citadel->setY(record.value("y").toDouble());
        citadel->setZ(record.value("z").toDouble());
        citadel->setFirstSeen(firstSeen);
        citadel->setLastSeen(lastSeen);
        citadel->setNew(false);

        return citadel;
    }

    void CitadelRepository::create() const
    {
        exec(QString{"CREATE TABLE IF NOT EXISTS %1 ("
            "id INTEGER PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "solar_system_id INTEGER NOT NULL,"
            "x REAL NOT NULL,"
            "y REAL NOT NULL,"
            "z REAL NOT NULL,"
            "last_seen DATETIME NOT NULL,"
            "first_seen DATETIME NOT NULL,"
            "public INTEGER NOT NULL"
        ")"}.arg(getTableName()));
    }

    void CitadelRepository::deleteAll() const
    {
        exec(QString{"DELETE FROM %1"}.arg(getTableName()));
    }

    QStringList CitadelRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "name"
            << "solar_system_id"
            << "x"
            << "y"
            << "z"
            << "last_seen"
            << "first_seen"
            << "public";
    }

    void CitadelRepository::bindValues(const Citadel &entity, QSqlQuery &query) const
    {
        if (entity.getId() != Citadel::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":name", entity.getName());
        query.bindValue(":solar_system_id", entity.getSolarSystemId());
        query.bindValue(":x", entity.getX());
        query.bindValue(":y", entity.getY());
        query.bindValue(":z", entity.getZ());
        query.bindValue(":last_seen", entity.getLastSeen());
        query.bindValue(":first_seen", entity.getFirstSeen());
        query.bindValue(":public", entity.isPublic());
    }

    void CitadelRepository::bindPositionalValues(const Citadel &entity, QSqlQuery &query) const
    {
        if (entity.getId() != Citadel::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getName());
        query.addBindValue(entity.getSolarSystemId());
        query.addBindValue(entity.getX());
        query.addBindValue(entity.getY());
        query.addBindValue(entity.getZ());
        query.addBindValue(entity.getLastSeen());
        query.addBindValue(entity.getFirstSeen());
        query.addBindValue(entity.isPublic());
    }
}
