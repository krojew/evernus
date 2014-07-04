#pragma once

#include <vector>

#include "Repository.h"
#include "ItemPrice.h"

namespace Evernus
{
    class ItemPriceRepository
        : public Repository<ItemPrice>
    {
    public:
        using Repository::Repository;
        virtual ~ItemPriceRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual ItemPrice populate(const QSqlRecord &record) const override;

        void create() const;

        ItemPrice findSellByTypeAndLocation(ItemPrice::TypeIdType typeId, ItemPrice::LocationIdType locationId) const;

        void batchStore(const std::vector<ItemPrice> &prices) const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const ItemPrice &entity, QSqlQuery &query) const override;
    };
}
