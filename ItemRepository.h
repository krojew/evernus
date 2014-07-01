#pragma once

#include <QHash>

#include "Repository.h"
#include "Item.h"

namespace Evernus
{
    class AssetList;

    class ItemRepository
        : public Repository<Item>
    {
    public:
        typedef QHash<QString, QVariantList> PropertyMap;

        using Repository::Repository;
        virtual ~ItemRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual Item populate(const QSqlRecord &record) const override;

        void create(const Repository<AssetList> &assetRepo) const;
        void batchStore(const PropertyMap &map) const;

        static void fillProperties(const Item &entity, PropertyMap &map);

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const Item &entity, QSqlQuery &query) const override;
    };
}
