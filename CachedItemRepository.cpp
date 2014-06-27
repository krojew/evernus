#include <QSqlRecord>
#include <QSqlQuery>

#include "CachedItemRepository.h"

namespace Evernus
{
    QString CachedItemRepository::getTableName() const
    {
        return "items";
    }

    QString CachedItemRepository::getIdColumn() const
    {
        return "id";
    }

    CachedItem CachedItemRepository::populate(const QSqlRecord &record) const
    {
        const auto locationId = record.value("location_id");
        const auto parentId = record.value("parent_id");

        CachedItem item{record.value("id").value<CachedItem::IdType>()};
        item.setParentId((parentId.isNull()) ? (CachedItem::ParentIdType{}) : (parentId.value<CachedItem::IdType>()));
        item.setTypeId(record.value("type_id").toUInt());
        item.setLocationId((locationId.isNull()) ? (ItemData::LocationIdType{}) : (locationId.value<ItemData::LocationIdType::value_type>()));
        item.setQuantity(record.value("quantity").toUInt());
        item.setNew(false);

        return item;
    }

    void CachedItemRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id BIGINT PRIMARY KEY,
            parent_id BIGINT NULL REFERENCES %1(id) ON UPDATE CASCADE ON DELETE CASCADE,
            type_id INTEGER NOT NULL,
            location_id BIGINT NULL,
            quantity INTEGER NOT NULL
        ))"}.arg(getTableName()));
    }

    QStringList CachedItemRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "parent_id"
            << "type_id"
            << "location_id"
            << "quantity";
    }

    void CachedItemRepository::bindValues(const CachedItem &entity, QSqlQuery &query) const
    {
        const auto locationId = entity.getLocationId();
        const auto parentId = entity.getParentId();

        query.bindValue(":id", entity.getId());
        query.bindValue(":parent_id", (parentId) ? (*parentId) : (QVariant{QVariant::ULongLong}));
        query.bindValue(":type_id", entity.getTypeId());
        query.bindValue(":location_id", (locationId) ? (*locationId) : (QVariant{QVariant::ULongLong}));
        query.bindValue(":quantity", entity.getQuantity());
    }
}
