#pragma once

#include "CachedConquerableStationList.h"
#include "CacheRepository.h"

namespace Evernus
{
    class CachedConquerableStationListRepository
        : public CacheRepository<CachedConquerableStationList>
    {
    public:
        using CacheRepository::CacheRepository;
        virtual ~CachedConquerableStationListRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual CachedConquerableStationList populate(const QSqlRecord &record) const override;

        void create() const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const CachedConquerableStationList &entity, QSqlQuery &query) const override;
    };
}
