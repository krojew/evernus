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

#include "ExternalOrderRepository.h"

namespace Evernus
{
    QString ExternalOrderRepository::getTableName() const
    {
        return "external_orders";
    }

    QString ExternalOrderRepository::getIdColumn() const
    {
        return "id";
    }

    ExternalOrderRepository::EntityPtr ExternalOrderRepository::populate(const QSqlRecord &record) const
    {
        auto dt = record.value("update_time").toDateTime();
        dt.setTimeSpec(Qt::UTC);

        auto externalOrder = std::make_shared<ExternalOrder>(record.value("id").value<ExternalOrder::IdType>());
        externalOrder->setType(static_cast<ExternalOrder::Type>(record.value("type").toInt()));
        externalOrder->setTypeId(record.value("type_id").value<ExternalOrder::TypeIdType>());
        externalOrder->setLocationId(record.value("location_id").value<ExternalOrder::LocationIdType>());
        externalOrder->setSolarSystemId(record.value("solar_system_id").toUInt());
        externalOrder->setRegionId(record.value("region_id").toUInt());
        externalOrder->setRange(record.value("range").toInt());
        externalOrder->setUpdateTime(dt);
        externalOrder->setValue(record.value("value").toDouble());
        externalOrder->setNew(false);

        return externalOrder;
    }

    void ExternalOrderRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY ASC,
            type TINYINT NOT NULL,
            type_id INTEGER NOT NULL,
            location_id BIGINT NOT NULL,
            solar_system_id INTEGER NOT NULL,
            region_id INTEGER NOT NULL,
            range INTEGER NOT NULL,
            update_time DATETIME NOT NULL,
            value DOUBLE NOT NULL
        ))"}.arg(getTableName()));
    }

    ExternalOrderRepository::EntityPtr ExternalOrderRepository::findSellByTypeAndLocation(ExternalOrder::TypeIdType typeId, ExternalOrder::LocationIdType locationId) const
    {
        return findByTypeAndLocation(typeId, locationId, ExternalOrder::Type::Sell);
    }

    ExternalOrderRepository::EntityPtr ExternalOrderRepository::findBuyByTypeAndLocation(ExternalOrder::TypeIdType typeId, ExternalOrder::LocationIdType locationId) const
    {
        return findByTypeAndLocation(typeId, locationId, ExternalOrder::Type::Buy);
    }

    QStringList ExternalOrderRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "type"
            << "type_id"
            << "location_id"
            << "solar_system_id"
            << "region_id"
            << "range"
            << "update_time"
            << "value";
    }

    void ExternalOrderRepository::bindValues(const ExternalOrder &entity, QSqlQuery &query) const
    {
        if (entity.getId() != ExternalOrder::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":type", static_cast<int>(entity.getType()));
        query.bindValue(":type_id", entity.getTypeId());
        query.bindValue(":location_id", entity.getLocationId());
        query.bindValue(":solar_system_id", entity.getSolarSystemId());
        query.bindValue(":region_id", entity.getRegionId());
        query.bindValue(":range", entity.getRange());
        query.bindValue(":update_time", entity.getUpdateTime());
        query.bindValue(":value", entity.getValue());
    }

    void ExternalOrderRepository::bindPositionalValues(const ExternalOrder &entity, QSqlQuery &query) const
    {
        if (entity.getId() != ExternalOrder::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(static_cast<int>(entity.getType()));
        query.addBindValue(entity.getTypeId());
        query.addBindValue(entity.getLocationId());
        query.addBindValue(entity.getSolarSystemId());
        query.addBindValue(entity.getRegionId());
        query.addBindValue(entity.getRange());
        query.addBindValue(entity.getUpdateTime());
        query.addBindValue(entity.getValue());
    }

    ExternalOrderRepository::EntityPtr ExternalOrderRepository
    ::findByTypeAndLocation(ExternalOrder::TypeIdType typeId, ExternalOrder::LocationIdType locationId, ExternalOrder::Type priceType) const
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
