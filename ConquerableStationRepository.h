#pragma once

#include "ConquerableStationList.h"
#include "Repository.h"

namespace Evernus
{
    class ConquerableStationRepository
        : public Repository<ConquerableStation>
    {
    public:
        using Repository::Repository;
        virtual ~ConquerableStationRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual ConquerableStation populate(const QSqlRecord &record) const override;

        void create() const;
        void batchStore(const ConquerableStationList &list) const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const ConquerableStation &entity, QSqlQuery &query) const override;
    };
}
