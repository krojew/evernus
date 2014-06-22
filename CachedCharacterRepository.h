#pragma once

#include "CachedCharacter.h"
#include "CacheRepository.h"

namespace Evernus
{
    class CachedCharacterRepository
        : public CacheRepository<CachedCharacter>
    {
    public:
        using CacheRepository::CacheRepository;
        virtual ~CachedCharacterRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual CachedCharacter populate(const QSqlRecord &record) const override;

        void create() const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const CachedCharacter &entity, QSqlQuery &query) const override;
    };
}
