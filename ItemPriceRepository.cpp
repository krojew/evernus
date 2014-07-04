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
            id INTEGER PRIMARY KEY,
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
