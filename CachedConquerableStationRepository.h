#pragma once

#include "CachedConquerableStation.h"
#include "Repository.h"

namespace Evernus
{
    class CachedConquerableStationListRepository;

    class CachedConquerableStationRepository
        : public Repository<CachedConquerableStation>
    {
    public:
        using Repository::Repository;
        virtual ~CachedConquerableStationRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual CachedConquerableStation populate(const QSqlRecord &record) const override;

        void create(const CachedConquerableStationListRepository &listRepo) const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const CachedConquerableStation &entity, QSqlQuery &query) const override;
    };
}
