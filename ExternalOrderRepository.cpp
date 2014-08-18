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

#include "MarketOrder.h"

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
        auto updateDt = record.value("update_time").toDateTime();
        updateDt.setTimeSpec(Qt::UTC);

        auto issuedDt = record.value("issued").toDateTime();
        issuedDt.setTimeSpec(Qt::UTC);

        auto externalOrder = std::make_shared<ExternalOrder>(record.value("id").value<ExternalOrder::IdType>());
        externalOrder->setType(static_cast<ExternalOrder::Type>(record.value("type").toInt()));
        externalOrder->setTypeId(record.value("type_id").value<ExternalOrder::TypeIdType>());
        externalOrder->setStationId(record.value("location_id").toUInt());
        externalOrder->setSolarSystemId(record.value("solar_system_id").toUInt());
        externalOrder->setRegionId(record.value("region_id").toUInt());
        externalOrder->setRange(record.value("range").toInt());
        externalOrder->setUpdateTime(updateDt);
        externalOrder->setPrice(record.value("value").toDouble());
        externalOrder->setVolumeEntered(record.value("volume_entered").toUInt());
        externalOrder->setVolumeRemaining(record.value("volume_remaining").toUInt());
        externalOrder->setMinVolume(record.value("min_volume").toUInt());
        externalOrder->setIssued(issuedDt);
        externalOrder->setDuration(record.value("duration").value<short>());
        externalOrder->setNew(false);

        return externalOrder;
    }

    void ExternalOrderRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id BIGINT PRIMARY KEY,
            type TINYINT NOT NULL,
            type_id INTEGER NOT NULL,
            location_id BIGINT NOT NULL,
            solar_system_id INTEGER NOT NULL,
            region_id INTEGER NOT NULL,
            range INTEGER NOT NULL,
            update_time DATETIME NOT NULL,
            value DOUBLE NOT NULL,
            volume_entered INTEGER NOT NULL,
            volume_remaining INTEGER NOT NULL,
            min_volume INTEGER NOT NULL,
            issued DATETIME NOT NULL,
            duration INTEGER NOT NULL
        ))"}.arg(getTableName()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_type_id_location ON %1(type_id, location_id)"}.arg(getTableName()));
        exec(QString{"CREATE INDEX IF NOT EXISTS %1_type_type_id_location ON %1(type, type_id, location_id)"}.arg(getTableName()));
        exec(QString{"CREATE INDEX IF NOT EXISTS %1_type_type_id_region ON %1(type, type_id, region_id)"}.arg(getTableName()));
    }

    ExternalOrderRepository::EntityPtr ExternalOrderRepository::findSellByTypeAndLocation(ExternalOrder::TypeIdType typeId,
                                                                                          uint locationId,
                                                                                          const Repository<MarketOrder> &orderRepo,
                                                                                          const Repository<MarketOrder> &corpOrderRepo) const
    {
        auto query = prepare(QString{R"(
            SELECT * FROM %1 WHERE type = ? AND type_id = ? AND location_id = ? AND id NOT IN
            (SELECT id FROM %2 WHERE state = ? UNION SELECT id FROM %3 WHERE state = ?)
            ORDER BY value ASC LIMIT 1)"}
            .arg(getTableName()).arg(orderRepo.getTableName()).arg(corpOrderRepo.getTableName()));
        query.addBindValue(static_cast<int>(ExternalOrder::Type::Sell));
        query.addBindValue(typeId);
        query.addBindValue(locationId);
        query.addBindValue(static_cast<int>(MarketOrder::State::Active));
        query.addBindValue(static_cast<int>(MarketOrder::State::Active));

        DatabaseUtils::execQuery(query);
        if (!query.next())
            throw NotFoundException{};

        return populate(query.record());
    }

    ExternalOrderRepository::EntityList ExternalOrderRepository::findBuyByTypeAndRegion(ExternalOrder::TypeIdType typeId,
                                                                                        uint regionId,
                                                                                        const Repository<MarketOrder> &orderRepo,
                                                                                        const Repository<MarketOrder> &corpOrderRepo) const
    {
        auto query = prepare(QString{R"(
            SELECT * FROM %1 WHERE type = ? AND type_id = ? AND region_id = ? AND id NOT IN
            (SELECT id FROM %2 WHERE state = ? UNION SELECT id FROM %3 WHERE state = ?)
        )"}.arg(getTableName()).arg(orderRepo.getTableName()).arg(corpOrderRepo.getTableName()));
        query.addBindValue(static_cast<int>(ExternalOrder::Type::Buy));
        query.addBindValue(typeId);
        query.addBindValue(regionId);
        query.addBindValue(static_cast<int>(MarketOrder::State::Active));
        query.addBindValue(static_cast<int>(MarketOrder::State::Active));

        DatabaseUtils::execQuery(query);

        EntityList result;

        const auto size = query.size();
        if (size > 0)
            result.reserve(size);

        while (query.next())
            result.emplace_back(populate(query.record()));

        return result;
    }

    void ExternalOrderRepository::removeObsolete(const ExternalOrderImporter::TypeLocationPairs &set) const
    {
        if (set.empty())
            return;

        const auto baseQuery = QString{"DELETE FROM %1 WHERE %2"}.arg(getTableName());
        const QString baseWhere{"(type_id = ? AND location_id = ?)"};

        const auto batchSize = 300;
        const auto batches = set.size() / batchSize;

        auto it = std::begin(set);

        QStringList batchWhere;
        for (auto i = 0; i < batchSize; ++i)
            batchWhere << baseWhere;

        const auto batchQuery = baseQuery.arg(batchWhere.join(" OR "));

        for (auto i = 0u; i < batches; ++i)
        {
            auto query = prepare(batchQuery);

            for (auto j = 0; j < batchSize; ++j)
            {
                query.addBindValue(it->first);
                query.addBindValue(it->second);

                ++it;
            }

            DatabaseUtils::execQuery(query);
        }

        const auto reminder = set.size() % batchSize;
        if (reminder == 0)
            return;

        QStringList reminderWhere;
        for (auto i = 0; i < reminder; ++i)
            reminderWhere << baseWhere;

        const auto reminderQuery = baseQuery.arg(reminderWhere.join(" OR "));
        auto query = prepare(reminderQuery);

        for (auto i = 0u; i < reminder; ++i)
        {
            query.addBindValue(it->first);
            query.addBindValue(it->second);

            ++it;
        }

        DatabaseUtils::execQuery(query);
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
            << "value"
            << "volume_entered"
            << "volume_remaining"
            << "min_volume"
            << "issued"
            << "duration";
    }

    void ExternalOrderRepository::bindValues(const ExternalOrder &entity, QSqlQuery &query) const
    {
        if (entity.getId() != ExternalOrder::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":type", static_cast<int>(entity.getType()));
        query.bindValue(":type_id", entity.getTypeId());
        query.bindValue(":location_id", entity.getStationId());
        query.bindValue(":solar_system_id", entity.getSolarSystemId());
        query.bindValue(":region_id", entity.getRegionId());
        query.bindValue(":range", entity.getRange());
        query.bindValue(":update_time", entity.getUpdateTime());
        query.bindValue(":value", entity.getPrice());
        query.bindValue(":volume_entered", entity.getVolumeEntered());
        query.bindValue(":volume_remaining", entity.getVolumeRemaining());
        query.bindValue(":min_volume", entity.getMinVolume());
        query.bindValue(":issued", entity.getIssued());
        query.bindValue(":duration", entity.getDuration());
    }

    void ExternalOrderRepository::bindPositionalValues(const ExternalOrder &entity, QSqlQuery &query) const
    {
        if (entity.getId() != ExternalOrder::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(static_cast<int>(entity.getType()));
        query.addBindValue(entity.getTypeId());
        query.addBindValue(entity.getStationId());
        query.addBindValue(entity.getSolarSystemId());
        query.addBindValue(entity.getRegionId());
        query.addBindValue(entity.getRange());
        query.addBindValue(entity.getUpdateTime());
        query.addBindValue(entity.getPrice());
        query.addBindValue(entity.getVolumeEntered());
        query.addBindValue(entity.getVolumeRemaining());
        query.addBindValue(entity.getMinVolume());
        query.addBindValue(entity.getIssued());
        query.addBindValue(entity.getDuration());
    }

    size_t ExternalOrderRepository::getMaxRowsPerInsert() const
    {
        return 71;
    }
}
