#pragma once

#include "Repository.h"
#include "EveType.h"

namespace Evernus
{
    class EveTypeRepository
        : public Repository<EveType>
    {
    public:
        using Repository::Repository;
        virtual ~EveTypeRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual EveType populate(const QSqlRecord &record) const override;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const EveType &entity, QSqlQuery &query) const override;
    };
}
