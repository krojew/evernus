#pragma once

#include "Repository.h"
#include "AssetList.h"

namespace Evernus
{
    class Character;

    class AssetListRepository
        : public Repository<AssetList>
    {
    public:
        using Repository::Repository;
        virtual ~AssetListRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual AssetList populate(const QSqlRecord &record) const override;

        void create(const Repository<Character> &itemRepo) const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const AssetList &entity, QSqlQuery &query) const override;
    };
}
