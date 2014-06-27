#pragma once

#include "MarketGroup.h"
#include "Repository.h"

namespace Evernus
{
    class MarketGroupRepository
        : public Repository<MarketGroup>
    {
    public:
        using Repository::Repository;
        virtual ~MarketGroupRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual MarketGroup populate(const QSqlRecord &record) const override;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const MarketGroup &entity, QSqlQuery &query) const override;
    };
}
