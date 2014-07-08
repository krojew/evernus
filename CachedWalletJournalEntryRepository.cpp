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

#include "CachedWalletJournalEntryRepository.h"

namespace Evernus
{
    QString CachedWalletJournalEntryRepository::getTableName() const
    {
        return "wallet_journal";
    }

    QString CachedWalletJournalEntryRepository::getIdColumn() const
    {
        return "id";
    }

    CachedWalletJournalEntry CachedWalletJournalEntryRepository::populate(const QSqlRecord &record) const
    {
        const auto argName = record.value("arg_name");
        const auto reason = record.value("reason");
        const auto taxReceiverId = record.value("tax_receiver_id");
        const auto taxAmount = record.value("tax_amount");

        auto cacheUntil = record.value("cache_until").toDateTime();
        cacheUntil.setTimeSpec(Qt::UTC);

        CachedWalletJournalEntry cachedWalletJournalEntry{record.value("id").value<CachedWalletJournalEntry::IdType>()};
        cachedWalletJournalEntry.setCacheUntil(cacheUntil);
        cachedWalletJournalEntry.setCharacterId(record.value("character_id").value<Character::IdType>());
        cachedWalletJournalEntry.setTimestamp(record.value("timestamp").toDateTime());
        cachedWalletJournalEntry.setRefTypeId(record.value("ref_type_id").toUInt());
        cachedWalletJournalEntry.setOwnerName1(record.value("owner_name_1").toString());
        cachedWalletJournalEntry.setOwnerId1(record.value("owner_id_1").toULongLong());
        cachedWalletJournalEntry.setOwnerName2(record.value("owner_name_2").toString());
        cachedWalletJournalEntry.setOwnerId2(record.value("owner_id_2").toULongLong());
        cachedWalletJournalEntry.setArgName((argName.isNull()) ? (CachedWalletJournalEntry::ArgType{}) : (argName.toString()));
        cachedWalletJournalEntry.setArgId(record.value("arg_id").toULongLong());
        cachedWalletJournalEntry.setAmount(record.value("amount").toDouble());
        cachedWalletJournalEntry.setBalance(record.value("balance").toDouble());
        cachedWalletJournalEntry.setReason((reason.isNull()) ? (CachedWalletJournalEntry::ReasonType{}) : (reason.toString()));
        cachedWalletJournalEntry.setTaxReceiverId((taxReceiverId.isNull()) ? (CachedWalletJournalEntry::TaxReceiverType{}) : (taxReceiverId.toULongLong()));
        cachedWalletJournalEntry.setTaxAmount((taxAmount.isNull()) ? (CachedWalletJournalEntry::TaxAmountType{}) : (taxAmount.toDouble()));
        cachedWalletJournalEntry.setNew(false);

        return cachedWalletJournalEntry;
    }

    void CachedWalletJournalEntryRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY,
            cache_until DATETIME NOT NULL,
            character_id BIGINT NOT NULL,
            timestamp DATETIME NOT NULL,
            ref_type_id INTEGER NOT NULL,
            owner_name_1 TEXT NOT NULL,
            owner_id_1 BIGINT NOT NULL,
            owner_name_2 TEXT NOT NULL,
            owner_id_2 BIGINT NOT NULL,
            arg_name TEXT NULL,
            arg_id BIGINT NOT NULL,
            amount NUMERIC NOT NULL,
            balance NUMERIC NOT NULL,
            reason TEXT NULL,
            tax_receiver_id BIGINT NULL,
            tax_amount NUMERIC NULL
        ))"}.arg(getTableName()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_cache_until ON %1(cache_until)"}.arg(getTableName()));
        exec(QString{"CREATE INDEX IF NOT EXISTS %1_character ON %1(character_id)"}.arg(getTableName()));
    }

    QStringList CachedWalletJournalEntryRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "cache_until"
            << "character_id"
            << "timestamp"
            << "ref_type_id"
            << "owner_name_1"
            << "owner_id_1"
            << "owner_name_2"
            << "owner_id_2"
            << "arg_name"
            << "arg_id"
            << "amount"
            << "balance"
            << "reason"
            << "tax_receiver_id"
            << "tax_amount";
    }

    void CachedWalletJournalEntryRepository::bindValues(const CachedWalletJournalEntry &entity, QSqlQuery &query) const
    {
        const auto argName = entity.getArgName();
        const auto reason = entity.getReason();
        const auto taxReceiverId = entity.getTaxReceiverId();
        const auto taxAmount = entity.getTaxAmount();

        if (entity.getId() != CachedWalletJournalEntry::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":cache_until", entity.getCacheUntil());
        query.bindValue(":character_id", entity.getCharacterId());
        query.bindValue(":timestamp", entity.getTimestamp());
        query.bindValue(":ref_type_id", entity.getRefTypeId());
        query.bindValue(":owner_name_1", entity.getOwnerName1());
        query.bindValue(":owner_id_1", entity.getOwnerId1());
        query.bindValue(":owner_name_2", entity.getOwnerName2());
        query.bindValue(":owner_id_2", entity.getOwnerId2());
        query.bindValue(":argName", (argName) ? (*argName) : (QVariant{QVariant::String}));
        query.bindValue(":arg_id", entity.getArgId());
        query.bindValue(":amount", entity.getAmount());
        query.bindValue(":balance", entity.getBalance());
        query.bindValue(":reason", (reason) ? (*reason) : (QVariant{QVariant::String}));
        query.bindValue(":tax_receiver_id", (taxReceiverId) ? (*taxReceiverId) : (QVariant{QVariant::ULongLong}));
        query.bindValue(":tax_amount", (taxAmount) ? (*taxAmount) : (QVariant{QVariant::Double}));
    }

    void CachedWalletJournalEntryRepository::bindPositionalValues(const CachedWalletJournalEntry &entity, QSqlQuery &query) const
    {
        const auto argName = entity.getArgName();
        const auto reason = entity.getReason();
        const auto taxReceiverId = entity.getTaxReceiverId();
        const auto taxAmount = entity.getTaxAmount();

        if (entity.getId() != CachedWalletJournalEntry::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getCacheUntil());
        query.addBindValue(entity.getCharacterId());
        query.addBindValue(entity.getTimestamp());
        query.addBindValue(entity.getRefTypeId());
        query.addBindValue(entity.getOwnerName1());
        query.addBindValue(entity.getOwnerId1());
        query.addBindValue(entity.getOwnerName2());
        query.addBindValue(entity.getOwnerId2());
        query.addBindValue((argName) ? (*argName) : (QVariant{QVariant::String}));
        query.addBindValue(entity.getArgId());
        query.addBindValue(entity.getAmount());
        query.addBindValue(entity.getBalance());
        query.addBindValue((reason) ? (*reason) : (QVariant{QVariant::String}));
        query.addBindValue((taxReceiverId) ? (*taxReceiverId) : (QVariant{QVariant::ULongLong}));
        query.addBindValue((taxAmount) ? (*taxAmount) : (QVariant{QVariant::Double}));
    }

    size_t CachedWalletJournalEntryRepository::getMaxRowsPerInsert() const
    {
        return 60;
    }
}
