#pragma once

#include "CachedAssetList.h"
#include "CacheRepository.h"

namespace Evernus
{
    class CachedAssetListRepository
        : public CacheRepository<CachedAssetList>
    {
    public:
        using CacheRepository::CacheRepository;
        virtual ~CachedAssetListRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual CachedAssetList populate(const QSqlRecord &record) const override;

        void create() const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const CachedAssetList &entity, QSqlQuery &query) const override;
    };
}
