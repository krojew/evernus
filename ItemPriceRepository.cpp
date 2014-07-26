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

    ItemPriceRepository::EntityPtr ItemPriceRepository::populate(const QSqlRecord &record) const
    {
        auto dt = record.value("update_time").toDateTime();
        dt.setTimeSpec(Qt::UTC);

        auto itemPrice = std::make_shared<ItemPrice>(record.value("id").value<ItemPrice::IdType>());
        itemPrice->setType(static_cast<ItemPrice::Type>(record.value("type").toInt()));
        itemPrice->setTypeId(record.value("type_id").value<ItemPrice::TypeIdType>());
        itemPrice->setLocationId(record.value("location_id").value<ItemPrice::LocationIdType>());
        itemPrice->setUpdateTime(dt);
        itemPrice->setValue(record.value("value").toDouble());
        itemPrice->setNew(false);

        return itemPrice;
    }

    void ItemPriceRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY ASC,
            type TINYINT NOT NULL,
            type_id INTEGER NOT NULL,
            location_id BIGINT NOT NULL,
            update_time DATETIME NOT NULL,
            value DOUBLE NOT NULL
        ))"}.arg(getTableName()));

        exec(QString{"CREATE UNIQUE INDEX IF NOT EXISTS %1_type_id_location_index ON %1(type, type_id, location_id)"}
            .arg(getTableName()));
    }

    ItemPriceRepository::EntityPtr ItemPriceRepository::findSellByTypeAndLocation(ItemPrice::TypeIdType typeId, ItemPrice::LocationIdType locationId) const
    {
        return findByTypeAndLocation(typeId, locationId, ItemPrice::Type::Sell);
    }

    ItemPriceRepository::EntityPtr ItemPriceRepository::findBuyByTypeAndLocation(ItemPrice::TypeIdType typeId, ItemPrice::LocationIdType locationId) const
    {
        return findByTypeAndLocation(typeId, locationId, ItemPrice::Type::Buy);
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
        if (entity.getId() != ItemPrice::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":type", static_cast<int>(entity.getType()));
        query.bindValue(":type_id", entity.getTypeId());
        query.bindValue(":location_id", entity.getLocationId());
        query.bindValue(":update_time", entity.getUpdateTime());
        query.bindValue(":value", entity.getValue());
    }

    void ItemPriceRepository::bindPositionalValues(const ItemPrice &entity, QSqlQuery &query) const
    {
        if (entity.getId() != ItemPrice::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(static_cast<int>(entity.getType()));
        query.addBindValue(entity.getTypeId());
        query.addBindValue(entity.getLocationId());
        query.addBindValue(entity.getUpdateTime());
        query.addBindValue(entity.getValue());
    }

    ItemPriceRepository::EntityPtr ItemPriceRepository
    ::findByTypeAndLocation(ItemPrice::TypeIdType typeId, ItemPrice::LocationIdType locationId, ItemPrice::Type priceType) const
    {
        auto query = prepare(QString{"SELECT * FROM %1 WHERE type = ? AND type_id = ? AND location_id = ?"}
            .arg(getTableName()));
        query.addBindValue(static_cast<int>(priceType));
        query.addBindValue(typeId);
        query.addBindValue(locationId);

        DatabaseUtils::execQuery(query);
        if (!query.next())
            throw NotFoundException{};

        return populate(query.record());
    }
}
