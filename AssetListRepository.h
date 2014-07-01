#pragma once

#include "Repository.h"
#include "AssetList.h"

namespace Evernus
{
    class ItemRepository;
    class Character;

    class AssetListRepository
        : public Repository<AssetList>
    {
    public:
        AssetListRepository(const QSqlDatabase &db, const ItemRepository &itemRepository);
        virtual ~AssetListRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual AssetList populate(const QSqlRecord &record) const override;

        void create(const Repository<Character> &characterRepo) const;

    private:
        const ItemRepository &mItemRepository;

        virtual QStringList getColumns() const override;
        virtual void bindValues(const AssetList &entity, QSqlQuery &query) const override;

        virtual void preStore(AssetList &entity) const override;
        virtual void postStore(AssetList &entity) const override;
    };
}
