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
#include "CachedAssetListRepository.h"

namespace Evernus
{
    QString CachedAssetListRepository::getTableName() const
    {
        return "asset_lists";
    }

    QString CachedAssetListRepository::getIdColumn() const
    {
        return "id";
    }

    CachedAssetList CachedAssetListRepository::populate(const QSqlRecord &record) const
    {
        auto cacheUntil = record.value("cache_until").toDateTime();
        cacheUntil.setTimeSpec(Qt::UTC);

        CachedAssetList list;
        list.setId(record.value("id").value<CachedAssetList::IdType>());
        list.setCharacterId(record.value("character_id").value<Character::IdType>());
        list.setCacheUntil(cacheUntil);
        list.setNew(false);

        return list;
    }

    void CachedAssetListRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY ASC,
            character_id BIGINT NOT NULL UNIQUE,
            cache_until DATETIME NOT NULL
        ))"}.arg(getTableName()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_cache_until ON %1(cache_until)"}.arg(getTableName()));
    }

    QStringList CachedAssetListRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "character_id"
            << "cache_until";
    }

    void CachedAssetListRepository::bindValues(const CachedAssetList &entity, QSqlQuery &query) const
    {
        if (!entity.isNew())
            query.bindValue(":id", entity.getId());

        query.bindValue(":character_id", entity.getCharacterId());
        query.bindValue(":cache_until", entity.getCacheUntil());
    }

    void CachedAssetListRepository::bindPositionalValues(const CachedAssetList &entity, QSqlQuery &query) const
    {
        query.addBindValue(entity.getId());
        query.addBindValue(entity.getCharacterId());
        query.addBindValue(entity.getCacheUntil());
    }
}
