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
#include <QTextStream>
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

        auto lastSeen = record.value("last_seen").toDateTime();
        lastSeen.setTimeSpec(Qt::UTC);

        MarketOrder marketOrder{record.value("id").value<MarketOrder::IdType>()};
        marketOrder.setCharacterId(record.value("character_id").value<Character::IdType>());
        marketOrder.setLocationId(record.value("location_id").toULongLong());
        marketOrder.setVolumeEntered(record.value("volume_entered").toUInt());
        marketOrder.setVolumeRemaining(record.value("volume_remaining").toUInt());
        marketOrder.setMinVolume(record.value("min_volume").toUInt());
        marketOrder.setDelta(record.value("delta").toInt());
        marketOrder.setState(static_cast<MarketOrder::State>(record.value("state").toInt()));
        marketOrder.setTypeId(record.value("type_id").value<EveType::IdType>());
        marketOrder.setRange(record.value("range").value<short>());
        marketOrder.setAccountKey(record.value("account_key").value<short>());
        marketOrder.setDuration(record.value("duration").value<short>());
        marketOrder.setEscrow(record.value("escrow").toDouble());
        marketOrder.setPrice(record.value("price").toDouble());
        marketOrder.setType(static_cast<MarketOrder::Type>(record.value("type").toInt()));
        marketOrder.setIssued(issued);
        marketOrder.setFirstSeen(firstSeen);
        marketOrder.setLastSeen(lastSeen);
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
            delta INTEGER NOT NULL DEFAULT 0,
            state TINYINT NOT NULL,
            type_id INTEGER NOT NULL,
            range INTEGER NOT NULL,
            account_key INTEGER NOT NULL,
            duration INTEGER NOT NULL,
            escrow NUMERIC NOT NULL,
            price NUMERIC NOT NULL,
            type TINYINT NOT NULL,
            issued DATETIME NOT NULL,
            first_seen DATETIME NOT NULL,
            last_seen DATETIME NULL
        ))"}.arg(getTableName()).arg(characterRepo.getTableName()).arg(characterRepo.getIdColumn()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_%2_index ON %1(character_id)"}.arg(getTableName()).arg(characterRepo.getTableName()));
        exec(QString{"CREATE INDEX IF NOT EXISTS %1_character_state ON %1(character_id, state)"}.arg(getTableName()));
        exec(QString{"CREATE INDEX IF NOT EXISTS %1_character_type_state ON %1(character_id, type, state)"}.arg(getTableName()));
    }

    MarketOrderRepository::AggrData MarketOrderRepository::getAggregatedData(Character::IdType characterId) const
    {
        auto query = prepare(QString{"SELECT type, COUNT(*), SUM(price) FROM %1 WHERE character_id = ? AND state = ? GROUP BY type"}
            .arg(getTableName()));
        query.bindValue(0, characterId);
        query.bindValue(1, static_cast<int>(MarketOrder::State::Active));

        DatabaseUtils::execQuery(query);

        AggrData data;

        for (auto i = 0; i < 2; ++i)
        {
            if (query.next())
            {
                SingleAggrData *singleData = nullptr;

                const auto type = static_cast<MarketOrder::Type>(query.value(0).toInt());
                if (type == MarketOrder::Type::Buy)
                    singleData = &data.mBuyData;
                else if (type == MarketOrder::Type::Sell)
                    singleData = &data.mSellData;

                if (singleData != nullptr)
                {
                    singleData->mCount = query.value(1).toUInt();
                    singleData->mPriceSum = query.value(2).toDouble();
                }
            }
        }

        return data;
    }

    MarketOrderRepository::OrderStateMap MarketOrderRepository::getOrderStatesAndVolumes(Character::IdType characterId) const
    {
        OrderStateMap result;

        auto query = prepare(QString{"SELECT %1, state, volume_remaining, first_seen FROM %2 WHERE character_id = ?"}
            .arg(getIdColumn())
            .arg(getTableName()));
        query.bindValue(0, characterId);

        DatabaseUtils::execQuery(query);

        while (query.next())
        {
            OrderState state;
            state.mState = static_cast<MarketOrder::State>(query.value(1).toInt());
            state.mVolumeRemaining = query.value(2).toUInt();
            state.mFirstSeen = query.value(3).toDateTime();
            state.mFirstSeen.setTimeSpec(Qt::UTC);

            result.emplace(query.value(0).value<MarketOrder::IdType>(), std::move(state));
        }

        return result;
    }

    void MarketOrderRepository::archive(const OrderIdList &list) const
    {
        QString queryStr;
        QTextStream stream(&queryStr);

        stream
            << "UPDATE "
            << getTableName()
            << " SET state = "
            << static_cast<int>(MarketOrder::State::Archived)
            << " WHERE "
            << getIdColumn()
            << " IN (";

        QStringList idList;
        for (auto i = 0; i < list.size(); ++i)
            idList << "?";

        stream
            << idList.join(", ")
            << ")";

        stream.flush();

        auto query = prepare(queryStr);
        for (const auto &id : list)
            query.addBindValue(id);

        DatabaseUtils::execQuery(query);
    }

    MarketOrderRepository::OrderList MarketOrderRepository::fetchForCharacter(Character::IdType characterId, MarketOrder::Type type) const
    {
        auto query = prepare(QString{"SELECT * FROM %1 WHERE character_id = ? AND type = ? AND state != ?"}.arg(getTableName()));
        query.bindValue(0, characterId);
        query.bindValue(1, static_cast<int>(type));
        query.bindValue(2, static_cast<int>(MarketOrder::State::Archived));

        DatabaseUtils::execQuery(query);

        OrderList result;

        const auto size = query.size();
        if (size > 0)
            result.reserve(size);

        while (query.next())
            result.emplace_back(populate(query.record()));

        return result;
    }

    MarketOrderRepository::OrderList MarketOrderRepository::fetchArchivedForCharacter(Character::IdType characterId) const
    {
        auto query = prepare(QString{"SELECT * FROM %1 WHERE character_id = ? AND state = ?"}.arg(getTableName()));
        query.bindValue(0, characterId);
        query.bindValue(1, static_cast<int>(MarketOrder::State::Archived));

        DatabaseUtils::execQuery(query);

        OrderList result;

        const auto size = query.size();
        if (size > 0)
            result.reserve(size);

        while (query.next())
            result.emplace_back(populate(query.record()));

        return result;
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
            << "first_seen"
            << "last_seen";
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
        query.bindValue(":issued", entity.getIssued());
        query.bindValue(":first_seen", entity.getFirstSeen());
        query.bindValue(":last_seen", entity.getLastSeen());
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
        query.addBindValue(entity.getIssued());
        query.addBindValue(entity.getFirstSeen());
        query.addBindValue(entity.getLastSeen());
    }

    size_t MarketOrderRepository::getMaxRowsPerInsert() const
    {
        return 55;
    }
}
