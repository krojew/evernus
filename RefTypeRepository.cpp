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
#include <QSqlRecord>
#include <QSqlQuery>

#include "RefTypeRepository.h"

namespace Evernus
{
    QString RefTypeRepository::getTableName() const
    {
        return "ref_types";
    }

    QString RefTypeRepository::getIdColumn() const
    {
        return "id";
    }

    RefTypeRepository::EntityPtr RefTypeRepository::populate(const QSqlRecord &record) const
    {
        auto refType = std::make_shared<RefType>(record.value("id").value<RefType::IdType>(), record.value("name").toString());
        refType->setNew(false);

        return refType;
    }

    void RefTypeRepository::create() const
    {
        exec(QString{"CREATE TABLE IF NOT EXISTS %1 ("
            "id INTEGER PRIMARY KEY,"
            "name TEXT NOT NULL"
        ")"}.arg(getTableName()));
    }

    QStringList RefTypeRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "name";
    }

    void RefTypeRepository::bindValues(const RefType &entity, QSqlQuery &query) const
    {
        if (entity.getId() != RefType::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":name", entity.getName());
    }

    void RefTypeRepository::bindPositionalValues(const RefType &entity, QSqlQuery &query) const
    {
        if (entity.getId() != RefType::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getName());
    }
}
