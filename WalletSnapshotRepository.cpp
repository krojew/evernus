#include <QSqlRecord>
#include <QSqlQuery>

#include "WalletSnapshotRepository.h"

namespace Evernus
{
    QString WalletSnapshotRepository::getTableName() const
    {
        return "wallet_snapshots";
    }

    QString WalletSnapshotRepository::getIdColumn() const
    {
        return "timestamp";
    }

    WalletSnapshot WalletSnapshotRepository::populate(const QSqlRecord &record) const
    {
        WalletSnapshot walletSnapshot{record.value("timestamp").value<WalletSnapshot::IdType>(), record.value("balance").toDouble()};
        walletSnapshot.setCharacterId(record.value("character_id").value<Character::IdType>());
        walletSnapshot.setNew(false);

        return walletSnapshot;
    }

    void WalletSnapshotRepository::create(const Repository<Character> &characterRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            timestamp TEXT PRIMARY KEY,
            character_id BIGINT NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE,
            balance DOUBLE NOT NULL
        ))"}.arg(getTableName()).arg(characterRepo.getTableName()).arg(characterRepo.getIdColumn()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_%2_index ON %1(character_id)"}.arg(getTableName()).arg(characterRepo.getTableName()));
    }

    QStringList WalletSnapshotRepository::getColumns() const
    {
        return QStringList{}
            << "timestamp"
            << "character_id"
            << "balance";
    }

    void WalletSnapshotRepository::bindValues(const WalletSnapshot &entity, QSqlQuery &query) const
    {
        query.bindValue(":timestamp", entity.getId());
        query.bindValue(":balance", entity.getBalance());
        query.bindValue(":character_id", entity.getCharacterId());
    }
}
