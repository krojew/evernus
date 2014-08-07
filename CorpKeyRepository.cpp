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
#include <QSqlRecord>
#include <QSqlQuery>

#include "CorpKeyRepository.h"

namespace Evernus
{
    QString CorpKeyRepository::getTableName() const
    {
        return "corp_keys";
    }

    QString CorpKeyRepository::getIdColumn() const
    {
        return "id";
    }

    CorpKeyRepository::EntityPtr CorpKeyRepository::populate(const QSqlRecord &record) const
    {
        auto corpKey = std::make_shared<CorpKey>(record.value("id").value<CorpKey::IdType>(), record.value("code").toString());
        corpKey->setCharacterId(record.value("character_id").value<Character::IdType>());
        corpKey->setNew(false);

        return corpKey;
    }

    void CorpKeyRepository::create(const Repository<Character> &characterRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY,
            character_id BIGINT NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE,
            code TEXT NOT NULL
        ))"}.arg(getTableName()).arg(characterRepo.getTableName()).arg(characterRepo.getIdColumn()));

        exec(QString{"CREATE UNIQUE INDEX IF NOT EXISTS %1_%2_index ON %1(character_id)"}.arg(getTableName()).arg(characterRepo.getTableName()));
    }

    CorpKeyRepository::EntityPtr CorpKeyRepository::fetchForCharacter(Character::IdType characterId) const
    {
        auto query = prepare(QString{"SELECT * FROM %1 WHERE character_id = ?"}.arg(getTableName()));
        query.bindValue(0, characterId);

        DatabaseUtils::execQuery(query);

        if (!query.next())
            throw NotFoundException{};

        return populate(query.record());
    }

    QStringList CorpKeyRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "character_id"
            << "code";
    }

    void CorpKeyRepository::bindValues(const CorpKey &entity, QSqlQuery &query) const
    {
        if (entity.getId() != CorpKey::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":character_id", entity.getCharacterId());
        query.bindValue(":code", entity.getCode());
    }

    void CorpKeyRepository::bindPositionalValues(const CorpKey &entity, QSqlQuery &query) const
    {
        if (entity.getId() != CorpKey::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getCharacterId());
        query.addBindValue(entity.getCode());
    }
}
