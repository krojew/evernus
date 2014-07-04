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

#include "CachedAssetListRepository.h"

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
        item.setListId(record.value("asset_list_id").value<CachedAssetList::IdType>());
        item.setParentId((parentId.isNull()) ? (CachedItem::ParentIdType{}) : (parentId.value<CachedItem::IdType>()));
        item.setTypeId(record.value("type_id").toUInt());
        item.setLocationId((locationId.isNull()) ? (ItemData::LocationIdType{}) : (locationId.value<ItemData::LocationIdType::value_type>()));
        item.setQuantity(record.value("quantity").toUInt());
        item.setNew(false);

        return item;
    }

    void CachedItemRepository::create(const CachedAssetListRepository &assetRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id BIGINT PRIMARY KEY,
            asset_list_id INTEGER NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE,
            parent_id BIGINT NULL REFERENCES %1(id) ON UPDATE CASCADE ON DELETE CASCADE,
            type_id INTEGER NOT NULL,
            location_id BIGINT NULL,
            quantity INTEGER NOT NULL
        ))"}.arg(getTableName()).arg(assetRepo.getTableName()).arg(assetRepo.getIdColumn()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_%2_index ON %1(asset_list_id)"}.arg(getTableName()).arg(assetRepo.getTableName()));
    }

    QStringList CachedItemRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "asset_list_id"
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
        query.bindValue(":asset_list_id", entity.getListId());
        query.bindValue(":parent_id", (parentId) ? (*parentId) : (QVariant{QVariant::ULongLong}));
        query.bindValue(":type_id", entity.getTypeId());
        query.bindValue(":location_id", (locationId) ? (*locationId) : (QVariant{QVariant::ULongLong}));
        query.bindValue(":quantity", entity.getQuantity());
    }
}
