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
#include "CachedCharacterListRepository.h"

namespace Evernus
{
    QString CachedCharacterListRepository::getTableName() const
    {
        return "character_lists";
    }

    QString CachedCharacterListRepository::getIdColumn() const
    {
        return "key_id";
    }

    CachedCharacterList CachedCharacterListRepository::populate(const QSqlRecord &record) const
    {
        const auto data = record.value("data").toByteArray();
        CachedCharacterList::CharacterList listData(data.size() / sizeof(CachedCharacterList::CharacterList::value_type));

        std::memcpy(listData.data(), data.constData(), data.size());

        auto cacheUntil = record.value("cache_until").toDateTime();
        cacheUntil.setTimeSpec(Qt::UTC);

        CachedCharacterList list;
        list.setId(record.value("key_id").value<CachedCharacterList::IdType>());
        list.setCacheUntil(cacheUntil);
        list.setCharacterList(std::move(listData));
        list.setNew(false);

        return list;
    }

    void CachedCharacterListRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            key_id INTEGER PRIMARY KEY,
            cache_until DATETIME NOT NULL,
            data BLOB NOT NULL
        ))"}.arg(getTableName()));
    }

    QStringList CachedCharacterListRepository::getColumns() const
    {
        return QStringList{}
            << "key_id"
            << "cache_until"
            << "data";
    }

    void CachedCharacterListRepository::bindValues(const CachedCharacterList &entity, QSqlQuery &query) const
    {
        const auto data = entity.getCharacterList();

        query.bindValue(":key_id", entity.getId());
        query.bindValue(":cache_until", entity.getCacheUntil());
        query.bindValue(":data", QByteArray{
            reinterpret_cast<const char *>(data.data()),
            static_cast<int>(data.size() * sizeof(CachedCharacterList::CharacterList::value_type))});
    }

    void CachedCharacterListRepository::bindPositionalValues(const CachedCharacterList &entity, QSqlQuery &query) const
    {
        const auto data = entity.getCharacterList();

        query.addBindValue(entity.getId());
        query.addBindValue(entity.getCacheUntil());
        query.addBindValue(QByteArray{
            reinterpret_cast<const char *>(data.data()),
            static_cast<int>(data.size() * sizeof(CachedCharacterList::CharacterList::value_type))});
    }
}
