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

#include "ContractRepository.h"

namespace Evernus
{
    ContractRepository::ContractRepository(bool corp, const QSqlDatabase &db)
        : Repository{db}
        , mCorp{corp}
    {
    }

    QString ContractRepository::getTableName() const
    {
        return "contracts";
    }

    QString ContractRepository::getIdColumn() const
    {
        return "id";
    }

    ContractRepository::EntityPtr ContractRepository::populate(const QSqlRecord &record) const
    {
        auto issued = record.value("issued").toDateTime();
        issued.setTimeSpec(Qt::UTC);

        auto expired = record.value("expired").toDateTime();
        expired.setTimeSpec(Qt::UTC);

        auto accepted = record.value("accepted").toDateTime();
        accepted.setTimeSpec(Qt::UTC);

        auto completed = record.value("completed").toDateTime();
        completed.setTimeSpec(Qt::UTC);

        auto contract = std::make_shared<Contract>(record.value("id").value<Contract::IdType>());
        contract->setIssuerId(record.value("issuer_id").value<Character::IdType>());
        contract->setIssuerCorpId(record.value("issuer_corp_id").toULongLong());
        contract->setAssigneeId(record.value("assignee_id").toULongLong());
        contract->setAcceptorId(record.value("acceptor_id").toULongLong());
        contract->setStartStationId(record.value("start_station_id").toUInt());
        contract->setEndStationId(record.value("end_station_id").toUInt());
        contract->setType(static_cast<Contract::Type>(record.value("type").toInt()));
        contract->setStatus(static_cast<Contract::Status>(record.value("status").toInt()));
        contract->setTitle(record.value("title").toString());
        contract->setForCorp(record.value("for_corp").toBool());
        contract->setIssued(issued);
        contract->setExpired(expired);
        contract->setAccepted(accepted);
        contract->setCompleted(completed);
        contract->setNumDays(record.value("num_days").toInt());
        contract->setPrice(record.value("price").toDouble());
        contract->setReward(record.value("reward").toDouble());
        contract->setCollateral(record.value("collateral").toDouble());
        contract->setBuyout(record.value("buyout").toDouble());
        contract->setVolume(record.value("volume").toDouble());
        contract->setNew(false);

        return contract;
    }

    void ContractRepository::create(const Repository<Character> &characterRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id BIGINT PRIMARY KEY,
            issuer_id BIGINT NOT NULL %2,
            issuer_corp_id BIGINT NOT NULL,
            assignee_id BIGINT NOT NULL,
            acceptor_id BIGINT NOT NULL,
            start_station_id INTEGER NOT NULL,
            end_station_id INTEGER NOT NULL,
            type TINYINT NOT NULL,
            status TINYINT NOT NULL,
            title TEXT NOT NULL,
            for_corp TINYINT NOT NULL,
            issued DATETIME NOT NULL,
            expired DATETIME NOT NULL,
            accepted DATETIME NOT NULL,
            completed DATETIME NOT NULL,
            num_days INTEGER NOT NULL,
            price DOUBLE NOT NULL,
            reward DOUBLE NOT NULL,
            collateral DOUBLE NOT NULL,
            buyout DOUBLE NOT NULL,
            volume DOUBLE NOT NULL
        ))"}.arg(getTableName()).arg(
            (mCorp) ? (QString{}) : (QString{"REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE"}.arg(characterRepo.getTableName()).arg(characterRepo.getIdColumn()))));
    }

    QStringList ContractRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "issuer_id"
            << "issuer_corp_id"
            << "assignee_id"
            << "acceptor_id"
            << "start_station_id"
            << "end_station_id"
            << "type"
            << "status"
            << "title"
            << "for_corp"
            << "issued"
            << "expired"
            << "accepted"
            << "completed"
            << "num_days"
            << "price"
            << "reward"
            << "collateral"
            << "buyout"
            << "volume";
    }

    void ContractRepository::bindValues(const Contract &entity, QSqlQuery &query) const
    {
        if (entity.getId() != Contract::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":issuer_id", entity.getIssuerId());
        query.bindValue(":issuer_corp_id", entity.getIssuerCorpId());
        query.bindValue(":assignee_id", entity.getAssigneeId());
        query.bindValue(":acceptor_id", entity.getAcceptorId());
        query.bindValue(":start_station_id", entity.getStartStationId());
        query.bindValue(":end_station_id", entity.getEndStationId());
        query.bindValue(":type", static_cast<int>(entity.getType()));
        query.bindValue(":status", static_cast<int>(entity.getStatus()));
        query.bindValue(":title", entity.getTitle());
        query.bindValue(":for_corp", entity.isForCorp());
        query.bindValue(":issued", entity.getIssued());
        query.bindValue(":expired", entity.getExpired());
        query.bindValue(":accepted", entity.getAccepted());
        query.bindValue(":completed", entity.getCompleted());
        query.bindValue(":num_days", entity.getNumDays());
        query.bindValue(":price", entity.getPrice());
        query.bindValue(":reward", entity.getReward());
        query.bindValue(":collateral", entity.getCollateral());
        query.bindValue(":buyout", entity.getBuyout());
        query.bindValue(":volume", entity.getVolume());
    }

    void ContractRepository::bindPositionalValues(const Contract &entity, QSqlQuery &query) const
    {
        if (entity.getId() != Contract::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getIssuerId());
        query.addBindValue(entity.getIssuerCorpId());
        query.addBindValue(entity.getAssigneeId());
        query.addBindValue(entity.getAcceptorId());
        query.addBindValue(entity.getStartStationId());
        query.addBindValue(entity.getEndStationId());
        query.addBindValue(static_cast<int>(entity.getType()));
        query.addBindValue(static_cast<int>(entity.getStatus()));
        query.addBindValue(entity.getTitle());
        query.addBindValue(entity.isForCorp());
        query.addBindValue(entity.getIssued());
        query.addBindValue(entity.getExpired());
        query.addBindValue(entity.getAccepted());
        query.addBindValue(entity.getCompleted());
        query.addBindValue(entity.getNumDays());
        query.addBindValue(entity.getPrice());
        query.addBindValue(entity.getReward());
        query.addBindValue(entity.getCollateral());
        query.addBindValue(entity.getBuyout());
        query.addBindValue(entity.getVolume());
    }
}
