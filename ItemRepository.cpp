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

#include "AssetList.h"

#include "ItemRepository.h"

namespace Evernus
{
    QString ItemRepository::getTableName() const
    {
        return "items";
    }

    QString ItemRepository::getIdColumn() const
    {
        return "id";
    }

    Item ItemRepository::populate(const QSqlRecord &record) const
    {
        const auto locationId = record.value("location_id");
        const auto parentId = record.value("parent_id");

        Item item{record.value("id").value<Item::IdType>()};
        item.setListId(record.value("asset_list_id").value<AssetList::IdType>());
        item.setParentId((parentId.isNull()) ? (Item::ParentIdType{}) : (parentId.value<Item::IdType>()));
        item.setTypeId(record.value("type_id").toUInt());
        item.setLocationId((locationId.isNull()) ? (ItemData::LocationIdType{}) : (locationId.value<ItemData::LocationIdType::value_type>()));
        item.setQuantity(record.value("quantity").toUInt());
        item.setNew(false);

        return item;
    }

    void ItemRepository::create(const Repository<AssetList> &assetRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY,
            asset_list_id INTEGER NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE,
            parent_id BIGINT NULL REFERENCES %1(id) ON UPDATE CASCADE ON DELETE CASCADE,
            type_id INTEGER NOT NULL,
            location_id BIGINT NULL,
            quantity INTEGER NOT NULL
        ))"}.arg(getTableName()).arg(assetRepo.getTableName()).arg(assetRepo.getIdColumn()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_%2_index ON %1(asset_list_id)"}.arg(getTableName()).arg(assetRepo.getTableName()));
    }

    void ItemRepository::batchStore(const PropertyMap &map) const
    {
        Q_ASSERT(!map.empty());

        const auto maxRowsPerInsert = 100;
        const auto totalRows = std::begin(map)->size();
        const auto batches = totalRows / maxRowsPerInsert;

        const auto columns = getColumns();
        const auto baseQueryStr = QString{"INSERT INTO %1 (%2) VALUES %3"}.arg(getTableName()).arg(columns.join(", "));

        QStringList columnBindings;
        for (auto i = 0; i < columns.count(); ++i)
            columnBindings << "?";

        const auto bindings = QString{"(%1)"}.arg(columnBindings.join(", "));

        QStringList batchBindings;
        for (auto i = 0; i < maxRowsPerInsert; ++i)
            batchBindings << bindings;

        const auto batchQueryStr = baseQueryStr.arg(batchBindings.join(", "));

        for (auto batch = 0; batch < batches; ++batch)
        {
            auto query = prepare(batchQueryStr);

            for (auto row = batch * maxRowsPerInsert; row < (batch + 1) * maxRowsPerInsert; ++row)
            {
                for (const auto &column : columns)
                    query.addBindValue(map[column][row]);
            }

            DatabaseUtils::execQuery(query);
        }

        QStringList restBindings;
        for (auto i = 0; i < totalRows % maxRowsPerInsert; ++i)
            restBindings << bindings;

        const auto restQueryStr = baseQueryStr.arg(restBindings.join(", "));
        auto query = prepare(restQueryStr);

        for (auto row = batches * maxRowsPerInsert; row < totalRows; ++row)
        {
            for (const auto &column : columns)
                query.addBindValue(map[column][row]);
        }

        DatabaseUtils::execQuery(query);
    }

    void ItemRepository::fillProperties(const Item &entity, PropertyMap &map)
    {
        const auto locationId = entity.getLocationId();
        const auto parentId = entity.getParentId();

        map["id"] << entity.getId();
        map["asset_list_id"] << entity.getListId();
        map["parent_id"] << ((parentId) ? (*parentId) : (QVariant{QVariant::ULongLong}));
        map["type_id"] << entity.getTypeId();
        map["location_id"] << ((locationId) ? (*locationId) : (QVariant{QVariant::ULongLong}));
        map["quantity"] << entity.getQuantity();

        for (const auto &item : entity)
            fillProperties(*item, map);
    }

    QStringList ItemRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "asset_list_id"
            << "parent_id"
            << "type_id"
            << "location_id"
            << "quantity";
    }

    void ItemRepository::bindValues(const Item &entity, QSqlQuery &query) const
    {
        const auto locationId = entity.getLocationId();
        const auto parentId = entity.getParentId();

        if (entity.getId() != Item::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":asset_list_id", entity.getListId());
        query.bindValue(":parent_id", (parentId) ? (*parentId) : (QVariant{QVariant::ULongLong}));
        query.bindValue(":type_id", entity.getTypeId());
        query.bindValue(":location_id", (locationId) ? (*locationId) : (QVariant{QVariant::ULongLong}));
        query.bindValue(":quantity", entity.getQuantity());
    }

    void ItemRepository::bindPositionalValues(const Item &entity, QSqlQuery &query) const
    {
        const auto locationId = entity.getLocationId();
        const auto parentId = entity.getParentId();

        if (entity.getId() != Item::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getListId());
        query.addBindValue((parentId) ? (*parentId) : (QVariant{QVariant::ULongLong}));
        query.addBindValue(entity.getTypeId());
        query.addBindValue((locationId) ? (*locationId) : (QVariant{QVariant::ULongLong}));
        query.addBindValue(entity.getQuantity());
    }
}
