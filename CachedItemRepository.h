#pragma once

#include "Repository.h"
#include "CachedItem.h"

namespace Evernus
{
    class CachedAssetListRepository;

    class CachedItemRepository
        : public Repository<CachedItem>
    {
    public:
        using Repository::Repository;
        virtual ~CachedItemRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual CachedItem populate(const QSqlRecord &record) const override;

        void create(const CachedAssetListRepository &assetRepo) const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const CachedItem &entity, QSqlQuery &query) const override;
    };
}
