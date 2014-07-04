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

    void ConquerableStationRepository::batchStore(const ConquerableStationList &list) const
    {
        if (list.empty())
            return;

        const auto maxRowsPerInsert = 100;
        const auto totalRows = list.size();
        const auto batches = totalRows / maxRowsPerInsert;
        const auto bindingStr = "(?, ?)";

        const auto binder = [](auto &query, const auto &row) {
            query.addBindValue(row->getId());
            query.addBindValue(row->getName());
        };

        const auto baseQueryStr = QString{"INSERT INTO %1 (id, name) VALUES %2"}.arg(getTableName());

        QStringList batchBindings;
        for (auto i = 0; i < maxRowsPerInsert; ++i)
            batchBindings << bindingStr;

        const auto batchQueryStr = baseQueryStr.arg(batchBindings.join(", "));

        for (auto batch = 0; batch < batches; ++batch)
        {
            auto query = prepare(batchQueryStr);

            const auto end = std::next(std::begin(list), (batch + 1) * maxRowsPerInsert);
            for (auto row = std::next(std::begin(list), batch * maxRowsPerInsert); row != end; ++row)
                binder(query, row);

            DatabaseUtils::execQuery(query);
        }

        QStringList restBindings;
        for (auto i = 0; i < totalRows % maxRowsPerInsert; ++i)
            restBindings << bindingStr;

        const auto restQueryStr = baseQueryStr.arg(restBindings.join(", "));
        auto query = prepare(restQueryStr);

        for (auto row = std::next(std::begin(list), batches * maxRowsPerInsert); row != std::end(list); ++row)
            binder(query, row);

        DatabaseUtils::execQuery(query);
    }

    QStringList ConquerableStationRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "name";
    }

    void ConquerableStationRepository::bindValues(const ConquerableStation &entity, QSqlQuery &query) const
    {
        query.bindValue(":id", entity.getId());
        query.bindValue(":name", entity.getName());
    }
}
