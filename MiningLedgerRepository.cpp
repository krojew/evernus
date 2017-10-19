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
#include "MiningLedgerRepository.h"

namespace Evernus
{
    QString MiningLedgerRepository::getTableName() const
    {
        return QStringLiteral("mining_ledger");
    }

    QString MiningLedgerRepository::getIdColumn() const
    {
        return QStringLiteral("id");
    }

    MiningLedgerRepository::EntityPtr MiningLedgerRepository::populate(const QSqlRecord &record) const
    {
        auto ledger = std::make_shared<MiningLedger>(record.value(getIdColumn()).value<MiningLedger::IdType>());
        ledger->setCharacterId(record.value(QStringLiteral("character_id")).value<Character::IdType>());
        ledger->setDate(record.value(QStringLiteral("date")).toDate());
        ledger->setQuantity(record.value(QStringLiteral("quantity")).toUInt());
        ledger->setSolarSystemId(record.value(QStringLiteral("solar_system_id")).toUInt());
        ledger->setTypeId(record.value(QStringLiteral("type_id")).value<EveType::IdType>());

        return ledger;
    }

    void MiningLedgerRepository::create(const Repository<Character> &characterRepo) const
    {
        exec(QStringLiteral("CREATE TABLE IF NOT EXISTS %1 ("
            "id BIGINT PRIMARY KEY,"
            "character_id BIGINT NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE,"
            "date DATE NOT NULL,"
            "quantity INTEGER NOT NULL,"
            "solar_system_id INTEGER NOT NULL,"
            "type_id INTEGER NOT NULL"
        ")").arg(getTableName()).arg(characterRepo.getTableName()).arg(characterRepo.getIdColumn()));
    }

    QStringList MiningLedgerRepository::getColumns() const
    {
        return {
            getIdColumn(),
            QStringLiteral("character_id"),
            QStringLiteral("date"),
            QStringLiteral("quantity"),
            QStringLiteral("solar_system_id"),
            QStringLiteral("type_id"),
        };
    }

    void MiningLedgerRepository::bindValues(const MiningLedger &entity, QSqlQuery &query) const
    {
        if (entity.getId() != MiningLedger::invalidId)
            query.bindValue(QStringLiteral(":id"), entity.getId());

        query.bindValue(QStringLiteral(":character_id"), entity.getCharacterId());
        query.bindValue(QStringLiteral(":date"), entity.getDate());
        query.bindValue(QStringLiteral(":quantity"), entity.getQuantity());
        query.bindValue(QStringLiteral(":solar_system_id"), entity.getSolarSystemId());
        query.bindValue(QStringLiteral(":type_id"), entity.getTypeId());
    }

    void MiningLedgerRepository::bindPositionalValues(const MiningLedger &entity, QSqlQuery &query) const
    {
        if (entity.getId() != MiningLedger::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getCharacterId());
        query.addBindValue(entity.getDate());
        query.addBindValue(entity.getQuantity());
        query.addBindValue(entity.getSolarSystemId());
        query.addBindValue(entity.getTypeId());
    }
}
