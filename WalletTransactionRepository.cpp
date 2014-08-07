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

#include "WalletTransactionRepository.h"

namespace Evernus
{
    WalletTransactionRepository::WalletTransactionRepository(bool corp, const QSqlDatabase &db)
        : Repository{db}
        , mCorp{corp}
    {
    }

    QString WalletTransactionRepository::getTableName() const
    {
        return (mCorp) ? ("corp_wallet_transactions") : ("wallet_transactions");
    }

    QString WalletTransactionRepository::getIdColumn() const
    {
        return "id";
    }

    WalletTransactionRepository::EntityPtr WalletTransactionRepository::populate(const QSqlRecord &record) const
    {
        auto timestamp = record.value("timestamp").toDateTime();
        timestamp.setTimeSpec(Qt::UTC);

        auto walletTransaction = std::make_shared<WalletTransaction>(record.value("id").value<WalletTransaction::IdType>());
        walletTransaction->setCharacterId(record.value("character_id").value<Character::IdType>());
        walletTransaction->setTimestamp(timestamp);
        walletTransaction->setQuantity(record.value("quantity").toUInt());
        walletTransaction->setTypeId(record.value("type_id").value<EveType::IdType>());
        walletTransaction->setPrice(record.value("price").toDouble());
        walletTransaction->setClientId(record.value("client_id").toULongLong());
        walletTransaction->setClientName(record.value("client_name").toString());
        walletTransaction->setLocationId(record.value("location_id").toULongLong());
        walletTransaction->setType(static_cast<WalletTransaction::Type>(record.value("type").toInt()));
        walletTransaction->setJournalId(record.value("journal_id").value<WalletJournalEntry::IdType>());
        walletTransaction->setIgnored(record.value("ignored").toBool());
        walletTransaction->setNew(false);

        return walletTransaction;
    }

    void WalletTransactionRepository::create(const Repository<Character> &characterRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY,
            character_id BIGINT NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE,
            timestamp DATETIME NOT NULL,
            quantity INTEGER NOT NULL,
            type_id INTEGER NOT NULL,
            price NUMERIC NOT NULL,
            client_id BIGINT NOT NULL,
            client_name TEXT NOT NULL,
            location_id BIGINT NOT NULL,
            type TINYINT NOT NULL,
            journal_id BIGINT NOT NULL,
            ignored TINYINT NOT NULL
        ))"}.arg(getTableName()).arg(characterRepo.getTableName()).arg(characterRepo.getIdColumn()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_%2_index ON %1(character_id)"}.arg(getTableName()).arg(characterRepo.getTableName()));
        exec(QString{"CREATE INDEX IF NOT EXISTS %1_timestamp ON %1(timestamp)"}.arg(getTableName()));
        exec(QString{"CREATE INDEX IF NOT EXISTS %1_character_timestamp_type ON %1(character_id, timestamp, type)"}.arg(getTableName()));
    }

    WalletTransaction::IdType WalletTransactionRepository::getLatestEntryId(Character::IdType characterId) const
    {
        auto query = prepare(QString{"SELECT MAX(%1) FROM %2 WHERE character_id = ?"}.arg(getIdColumn()).arg(getTableName()));
        query.bindValue(0, characterId);

        DatabaseUtils::execQuery(query);

        query.next();
        return query.value(0).value<WalletTransaction::IdType>();
    }

    void WalletTransactionRepository::setIgnored(WalletTransaction::IdType id, bool ignored) const
    {
        auto query = prepare(QString{"UPDATE %1 SET ignored = ? WHERE %2 = ?"}.arg(getTableName()).arg(getIdColumn()));
        query.bindValue(0, ignored);
        query.bindValue(1, id);

        DatabaseUtils::execQuery(query);
    }

    void WalletTransactionRepository::deleteOldEntires(const QDateTime &from) const
    {
        auto query = prepare(QString{"DELETE FROM %1 WHERE timestamp < ?"}.arg(getTableName()));
        query.bindValue(0, from);

        DatabaseUtils::execQuery(query);
    }

    WalletTransactionRepository::EntityList WalletTransactionRepository
    ::fetchForCharacterInRange(Character::IdType characterId,
                               const QDateTime &from,
                               const QDateTime &till,
                               EntryType type,
                               EveType::IdType typeId) const
    {
        QString queryStr;
        if (type == EntryType::All)
            queryStr = "SELECT * FROM %1 WHERE character_id = ? AND timestamp BETWEEN ? AND ?";
        else
            queryStr = "SELECT * FROM %1 WHERE character_id = ? AND timestamp BETWEEN ? AND ? AND type = ?";

        if (typeId != EveType::invalidId)
            queryStr += " AND type_id = ?";

        auto query = prepare(queryStr.arg(getTableName()));
        query.addBindValue(characterId);
        query.addBindValue(from);
        query.addBindValue(till);

        if (type != EntryType::All)
            query.addBindValue(static_cast<int>((type == EntryType::Buy) ? (WalletTransaction::Type::Buy) : (WalletTransaction::Type::Sell)));

        if (typeId != EveType::invalidId)
            query.addBindValue(typeId);

        DatabaseUtils::execQuery(query);

        const auto size = query.size();

        EntityList result;
        if (size > 0)
            result.reserve(size);

        while (query.next())
            result.emplace_back(populate(query.record()));

        return result;
    }

    QStringList WalletTransactionRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "character_id"
            << "timestamp"
            << "quantity"
            << "type_id"
            << "price"
            << "client_id"
            << "client_name"
            << "location_id"
            << "type"
            << "journal_id"
            << "ignored";
    }

    void WalletTransactionRepository::bindValues(const WalletTransaction &entity, QSqlQuery &query) const
    {
        if (entity.getId() != WalletTransaction::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":character_id", entity.getCharacterId());
        query.bindValue(":timestamp", entity.getTimestamp());
        query.bindValue(":quantity", entity.getQuantity());
        query.bindValue(":type_id", entity.getTypeId());
        query.bindValue(":price", entity.getPrice());
        query.bindValue(":client_id", entity.getClientId());
        query.bindValue(":client_name", entity.getClientName());
        query.bindValue(":location_id", entity.getLocationId());
        query.bindValue(":type", static_cast<int>(entity.getType()));
        query.bindValue(":journal_id", entity.getJournalId());
        query.bindValue(":ignored", entity.isIgnored());
    }

    void WalletTransactionRepository::bindPositionalValues(const WalletTransaction &entity, QSqlQuery &query) const
    {
        if (entity.getId() != WalletTransaction::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getCharacterId());
        query.addBindValue(entity.getTimestamp());
        query.addBindValue(entity.getQuantity());
        query.addBindValue(entity.getTypeId());
        query.addBindValue(entity.getPrice());
        query.addBindValue(entity.getClientId());
        query.addBindValue(entity.getClientName());
        query.addBindValue(entity.getLocationId());
        query.addBindValue(static_cast<int>(entity.getType()));
        query.addBindValue(entity.getJournalId());
        query.addBindValue(entity.isIgnored());
    }

    size_t WalletTransactionRepository::getMaxRowsPerInsert() const
    {
        return 80;
    }
}
