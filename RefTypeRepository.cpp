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

    RefType RefTypeRepository::populate(const QSqlRecord &record) const
    {
        RefType refType{record.value("id").value<RefType::IdType>(), record.value("name").toString()};
        refType.setNew(false);

        return refType;
    }

    void RefTypeRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL
        ))"}.arg(getTableName()));
    }

    void RefTypeRepository::batchStore(const RefTypeList &refs) const
    {
        if (refs.empty())
            return;

        const auto maxRowsPerInsert = 100;
        const auto totalRows = refs.size();
        const auto batches = totalRows / maxRowsPerInsert;
        const auto bindingStr = "(?, ?)";

        const auto binder = [](auto &query, const auto &row) {
            query.addBindValue(row->getId());
            query.addBindValue(row->getName());
        };

        const auto baseQueryStr = QString{"REPLACE INTO %1 (%2, name) VALUES %3"}
            .arg(getTableName())
            .arg(getIdColumn());

        QStringList batchBindings;
        for (auto i = 0; i < maxRowsPerInsert; ++i)
            batchBindings << bindingStr;

        const auto batchQueryStr = baseQueryStr.arg(batchBindings.join(", "));

        for (auto batch = 0; batch < batches; ++batch)
        {
            auto query = prepare(batchQueryStr);

            const auto end = std::next(std::begin(refs), (batch + 1) * maxRowsPerInsert);
            for (auto row = std::next(std::begin(refs), batch * maxRowsPerInsert); row != end; ++row)
                binder(query, row);

            DatabaseUtils::execQuery(query);
        }

        QStringList restBindings;
        for (auto i = 0; i < totalRows % maxRowsPerInsert; ++i)
            restBindings << bindingStr;

        const auto restQueryStr = baseQueryStr.arg(restBindings.join(", "));
        auto query = prepare(restQueryStr);

        for (auto row = std::next(std::begin(refs), batches * maxRowsPerInsert); row != std::end(refs); ++row)
            binder(query, row);

        DatabaseUtils::execQuery(query);
    }

    QStringList RefTypeRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "name";
    }

    void RefTypeRepository::bindValues(const RefType &entity, QSqlQuery &query) const
    {
        query.bindValue(":id", entity.getId());
        query.bindValue(":name", entity.getName());
    }
}
