#pragma once

#include "Repository.h"
#include "WalletSnapshot.h"

namespace Evernus
{
    class Character;

    class WalletSnapshotRepository
        : public Repository<WalletSnapshot>
    {
    public:
        using Repository::Repository;
        virtual ~WalletSnapshotRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual WalletSnapshot populate(const QSqlRecord &record) const override;

        void create(const Repository<Character> &characterRepo) const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const WalletSnapshot &entity, QSqlQuery &query) const override;
    };
}
