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

#include "MarketOrderRepository.h"

namespace Evernus
{
    QString MarketOrderRepository::getTableName() const
    {
        return "market_orders";
    }

    QString MarketOrderRepository::getIdColumn() const
    {
        return "id";
    }

    MarketOrder MarketOrderRepository::populate(const QSqlRecord &record) const
    {
        auto issued = record.value("issued").toDateTime();
        issued.setTimeSpec(Qt::UTC);

        auto firstSeen = record.value("first_seen").toDateTime();
        firstSeen.setTimeSpec(Qt::UTC);

        MarketOrder marketOrder{record.value("id").value<MarketOrder::IdType>()};
        marketOrder.setCharacterId(record.value("character_id").value<Character::IdType>());
        marketOrder.setLocationId(record.value("location_id").toULongLong());
        marketOrder.setVolumeEntered(record.value("volume_entered").toUInt());
        marketOrder.setVolumeRemaining(record.value("volume_remaining").toUInt());
        marketOrder.setMinVolume(record.value("min_volume").toUInt());
        marketOrder.setDelta(record.value("delta").toUInt());
        marketOrder.setState(static_cast<MarketOrder::State>(record.value("state").toInt()));
        marketOrder.setTypeId(record.value("type_id").value<EveType::IdType>());
        marketOrder.setRange(record.value("delta").value<short>());
        marketOrder.setAccountKey(record.value("account_key").value<short>());
        marketOrder.setDuration(record.value("duration").value<short>());
        marketOrder.setEscrow(record.value("escrow").toDouble());
        marketOrder.setPrice(record.value("price").toDouble());
        marketOrder.setType(static_cast<MarketOrder::Type>(record.value("type").toInt()));
        marketOrder.setIssued(issued);
        marketOrder.setFirstSeen(firstSeen);
        marketOrder.setNew(false);

        return marketOrder;
    }

    void MarketOrderRepository::create(const Repository<Character> &characterRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id BIGINT PRIMARY KEY,
            character_id BIGINT NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE,
            location_id BIGINT NOT NULL,
            volume_entered INTEGER NOT NULL,
            volume_remaining INTEGER NOT NULL,
            min_volume INTEGER NOT NULL,
            delta INTEGER NOT NULL,
            state TINYINT NOT NULL,
            type_id INTEGER NOT NULL,
            range INTEGER NOT NULL,
            account_key INTEGER NOT NULL,
            duration INTEGER NOT NULL,
            escrow NUMERIC NOT NULL,
            price NUMERIC NOT NULL,
            type TINYINT NOT NULL,
            issued DATETIME NOT NULL,
            first_seen DATETIME NOT NULL
        ))"}.arg(getTableName()).arg(characterRepo.getTableName()).arg(characterRepo.getIdColumn()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_%2_index ON %1(character_id)"}.arg(getTableName()).arg(characterRepo.getTableName()));
    }

    QStringList MarketOrderRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "character_id"
            << "location_id"
            << "volume_entered"
            << "volume_remaining"
            << "min_volume"
            << "delta"
            << "state"
            << "type_id"
            << "range"
            << "account_key"
            << "duration"
            << "escrow"
            << "price"
            << "type"
            << "issued"
            << "first_seen";
    }

    void MarketOrderRepository::bindValues(const MarketOrder &entity, QSqlQuery &query) const
    {
        if (entity.getId() != MarketOrder::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":character_id", entity.getCharacterId());
        query.bindValue(":location_id", entity.getLocationId());
        query.bindValue(":volume_entered", entity.getVolumeEntered());
        query.bindValue(":volume_remaining", entity.getVolumeRemaining());
        query.bindValue(":min_volume", entity.getMinVolume());
        query.bindValue(":delta", entity.getDelta());
        query.bindValue(":state", static_cast<int>(entity.getState()));
        query.bindValue(":type_id", entity.getTypeId());
        query.bindValue(":range", entity.getRange());
        query.bindValue(":account_key", entity.getAccountKey());
        query.bindValue(":duration", entity.getDuration());
        query.bindValue(":escrow", entity.getEscrow());
        query.bindValue(":price", entity.getPrice());
        query.bindValue(":type", static_cast<int>(entity.getType()));
        query.bindValue(":issued", entity.getPrice());
        query.bindValue(":first_seen", entity.getFirstSeen());
    }

    void MarketOrderRepository::bindPositionalValues(const MarketOrder &entity, QSqlQuery &query) const
    {
        if (entity.getId() != MarketOrder::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getCharacterId());
        query.addBindValue(entity.getLocationId());
        query.addBindValue(entity.getVolumeEntered());
        query.addBindValue(entity.getVolumeRemaining());
        query.addBindValue(entity.getMinVolume());
        query.addBindValue(entity.getDelta());
        query.addBindValue(static_cast<int>(entity.getState()));
        query.addBindValue(entity.getTypeId());
        query.addBindValue(entity.getRange());
        query.addBindValue(entity.getAccountKey());
        query.addBindValue(entity.getDuration());
        query.addBindValue(entity.getEscrow());
        query.addBindValue(entity.getPrice());
        query.addBindValue(static_cast<int>(entity.getType()));
        query.addBindValue(entity.getPrice());
        query.addBindValue(entity.getFirstSeen());
    }
}
