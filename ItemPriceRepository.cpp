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

#include "ItemPriceRepository.h"

namespace Evernus
{
    QString ItemPriceRepository::getTableName() const
    {
        return "item_prices";
    }

    QString ItemPriceRepository::getIdColumn() const
    {
        return "id";
    }

    ItemPrice ItemPriceRepository::populate(const QSqlRecord &record) const
    {
        ItemPrice itemPrice{record.value("id").value<ItemPrice::IdType>()};
        itemPrice.setType(static_cast<ItemPrice::Type>(record.value("type").toInt()));
        itemPrice.setTypeId(record.value("type_id").value<ItemPrice::TypeIdType>());
        itemPrice.setLocationId(record.value("location_id").value<ItemPrice::LocationIdType>());
        itemPrice.setUpdateTime(record.value("update_time").toDateTime());
        itemPrice.setValue(record.value("value").toDouble());
        itemPrice.setNew(false);

        return itemPrice;
    }

    void ItemPriceRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY ASC,
            type TINYINT NOT NULL,
            type_id INTEGER NOT NULL,
            location_id BIGINT NOT NULL,
            update_time TEXT NOT NULL,
            value DOUBLE NOT NULL
        ))"}.arg(getTableName()));

        exec(QString{"CREATE UNIQUE INDEX IF NOT EXISTS %1_type_id_location_index ON %1(type, type_id, location_id)"}
            .arg(getTableName()));
    }

    ItemPrice ItemPriceRepository::findSellByTypeAndLocation(ItemPrice::TypeIdType typeId, ItemPrice::LocationIdType locationId) const
    {
        auto query = prepare(QString{"SELECT * FROM %1 WHERE type = ? AND type_id = ? AND location_id = ?"}
            .arg(getTableName()));
        query.addBindValue(static_cast<int>(ItemPrice::Type::Sell));
        query.addBindValue(typeId);
        query.addBindValue(locationId);

        DatabaseUtils::execQuery(query);
        if (!query.next())
            throw NotFoundException{};

        return populate(query.record());
    }

    void ItemPriceRepository::batchStore(const std::vector<ItemPrice> &prices) const
    {
        if (prices.empty())
            return;

        const auto maxRowsPerInsert = 100;
        const auto totalRows = prices.size();
        const auto batches = totalRows / maxRowsPerInsert;
        const auto bindingStr = "(?, ?, ?, ?, ?)";

        const auto binder = [](auto &query, const auto &row) {
            query.addBindValue(static_cast<int>(row->getType()));
            query.addBindValue(row->getTypeId());
            query.addBindValue(row->getLocationId());
            query.addBindValue(row->getUpdateTime());
            query.addBindValue(row->getValue());
        };

        const auto baseQueryStr = QString{"INSERT INTO %1 (type, type_id, location_id, update_time, value) VALUES %2"}
            .arg(getTableName());

        QStringList batchBindings;
        for (auto i = 0; i < maxRowsPerInsert; ++i)
            batchBindings << bindingStr;

        const auto batchQueryStr = baseQueryStr.arg(batchBindings.join(", "));

        for (auto batch = 0; batch < batches; ++batch)
        {
            auto query = prepare(batchQueryStr);

            const auto end = std::next(std::begin(prices), (batch + 1) * maxRowsPerInsert);
            for (auto row = std::next(std::begin(prices), batch * maxRowsPerInsert); row != end; ++row)
                binder(query, row);

            DatabaseUtils::execQuery(query);
        }

        QStringList restBindings;
        for (auto i = 0; i < totalRows % maxRowsPerInsert; ++i)
            restBindings << bindingStr;

        const auto restQueryStr = baseQueryStr.arg(restBindings.join(", "));
        auto query = prepare(restQueryStr);

        for (auto row = std::next(std::begin(prices), batches * maxRowsPerInsert); row != std::end(prices); ++row)
            binder(query, row);

        DatabaseUtils::execQuery(query);
    }

    QStringList ItemPriceRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "type"
            << "type_id"
            << "location_id"
            << "update_time"
            << "value";
    }

    void ItemPriceRepository::bindValues(const ItemPrice &entity, QSqlQuery &query) const
    {
        query.bindValue(":id", entity.getId());
        query.bindValue(":type", static_cast<int>(entity.getType()));
        query.bindValue(":type_id", entity.getTypeId());
        query.bindValue(":location_id", entity.getLocationId());
        query.bindValue(":update_time", entity.getUpdateTime());
        query.bindValue(":value", entity.getValue());
    }
}
