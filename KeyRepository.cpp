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

#include "KeyRepository.h"

namespace Evernus
{
    QString KeyRepository::getTableName() const
    {
        return "keys";
    }

    QString KeyRepository::getIdColumn() const
    {
        return "id";
    }

    KeyRepository::EntityPtr KeyRepository::populate(const QSqlRecord &record) const
    {
        auto key = std::make_shared<Key>(record.value("id").value<Key::IdType>(), record.value("code").toString());
        key->setNew(false);

        return key;
    }

    void KeyRepository::create() const
    {
        exec(QString{"CREATE TABLE IF NOT EXISTS %1 ("
            "id INTEGER PRIMARY KEY,"
            "code TEXT NOT NULL"
        ")"}.arg(getTableName()));
    }

    QStringList KeyRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "code";
    }

    void KeyRepository::bindValues(const Key &entity, QSqlQuery &query) const
    {
        if (entity.getId() != Key::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":code", entity.getCode());
    }

    void KeyRepository::bindPositionalValues(const Key &entity, QSqlQuery &query) const
    {
        if (entity.getId() != Key::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getCode());
    }
}
