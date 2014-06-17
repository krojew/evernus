#pragma once

#include "Repository.h"
#include "Key.h"

namespace Evernus
{
    class KeyRepository
        : public Repository<Key>
    {
    public:
        using Repository::Repository;
        virtual ~KeyRepository() = default;

        virtual QString getTableName() const override;

        void create() const;

    private:
        virtual Key populate(const QSqlQuery &query) const override;
        virtual QStringList getColumns() const override;
        virtual void bindValues(const Key &entity, QSqlQuery &query) const override;
    };
}
