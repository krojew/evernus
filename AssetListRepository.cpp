/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <unordered_map>

#include <QSqlRecord>
#include <QSqlQuery>

#include "ItemRepository.h"

#include "AssetListRepository.h"

namespace Evernus
{
    AssetListRepository::AssetListRepository(const QSqlDatabase &db, const ItemRepository &itemRepository)
        : Repository{db}
        , mItemRepository{itemRepository}
    {
    }

    QString AssetListRepository::getTableName() const
    {
        return "asset_lists";
    }

    QString AssetListRepository::getIdColumn() const
    {
        return "id";
    }

    AssetList AssetListRepository::populate(const QSqlRecord &record) const
    {
        AssetList assetList{record.value("id").value<AssetList::IdType>()};
        assetList.setCharacterId(record.value("character_id").value<Character::IdType>());
        assetList.setNew(false);

        return assetList;
    }

    void AssetListRepository::create(const Repository<Character> &characterRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY ASC,
            character_id BIGINT NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE
        ))"}.arg(getTableName()).arg(characterRepo.getTableName()).arg(characterRepo.getIdColumn()));

        exec(QString{"CREATE UNIQUE INDEX IF NOT EXISTS %1_%2_index ON %1(character_id)"}.arg(getTableName()).arg(characterRepo.getTableName()));
    }

    AssetList AssetListRepository::fetchForCharacter(Character::IdType id) const
    {
        auto query = prepare(QString{"SELECT * FROM %1 WHERE character_id = ?"}.arg(getTableName()));
        query.bindValue(0, id);

        DatabaseUtils::execQuery(query);
        query.next();

        auto assets = populate(query.record());

        query = mItemRepository.prepare(QString{"SELECT * FROM %1 WHERE asset_list_id = ?"}.arg(mItemRepository.getTableName()));
        query.bindValue(0, assets.getId());

        DatabaseUtils::execQuery(query);

        std::unordered_map<Item::IdType, Item *> itemMap;
        std::vector<std::unique_ptr<Item>> items;

        while (query.next())
        {
            auto item = mItemRepository.populate(query.record());
            auto itemPtr = std::make_unique<Item>(std::move(item));

            itemMap[item.getId()] = itemPtr.get();
            items.emplace_back(std::move(itemPtr));
        }

        for (auto &item : items)
        {
            const auto parentId = item->getParentId();
            if (parentId && itemMap.find(*parentId) != std::end(itemMap))
                itemMap[*parentId]->addItem(std::move(item));
        }

        for (auto &item : items)
        {
            if (item)
                assets.addItem(std::move(item));
        }

        return assets;
    }

    QStringList AssetListRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "character_id";
    }

    void AssetListRepository::bindValues(const AssetList &entity, QSqlQuery &query) const
    {
        query.bindValue(":id", entity.getId());
        query.bindValue(":character_id", entity.getCharacterId());
    }

    void AssetListRepository::preStore(AssetList &entity) const
    {
        if (!entity.isNew())
        {
            auto query = mItemRepository.prepare(QString{"DELETE FROM %1 WHERE asset_list_id = ?"}.arg(mItemRepository.getTableName()));
            query.bindValue(0, entity.getId());

            DatabaseUtils::execQuery(query);
        }
    }

    void AssetListRepository::postStore(AssetList &entity) const
    {
        for (const auto &item : entity)
            item->setListId(entity.getId());

        ItemRepository::PropertyMap map;

        for (const auto &item : entity)
            ItemRepository::fillProperties(*item, map);

        mItemRepository.batchStore(map);
    }
}
