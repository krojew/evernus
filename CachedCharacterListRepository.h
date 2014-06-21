#pragma once

#include "CachedCharacterList.h"
#include "CacheRepository.h"

namespace Evernus
{
    class CachedCharacterListRepository
        : public CacheRepository<CachedCharacterList>
    {
    public:
        using CacheRepository::CacheRepository;
        virtual ~CachedCharacterListRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual CachedCharacterList populate(const QSqlRecord &record) const override;

        void create() const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const CachedCharacterList &entity, QSqlQuery &query) const override;
    };
}
