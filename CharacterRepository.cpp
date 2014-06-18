#include <QSqlQuery>

#include "KeyRepository.h"

#include "CharacterRepository.h"

namespace Evernus
{
    void CharacterRepository::create(const KeyRepository &keyRepository) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id BIGINT PRIMARY KEY,
            key_id BITINT NULL REFERENCES %2(id) ON UPDATE SET NULL ON DELETE SET NULL,
            name TEXT NOT NULL,
            enabled TINYINT NOT NULL
        ))"}.arg(getTableName()).arg(keyRepository.getTableName()));
    }

    QString CharacterRepository::getTableName() const
    {
        return "characters";
    }

    Character CharacterRepository::populate(const QSqlRecord &record) const
    {
        Character character{};
        character.setNew(false);

        return character;
    }

    QStringList CharacterRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "key_id"
            << "name"
            << "enabled";
    }

    QString CharacterRepository::getIdColumn() const
    {
        return "id";
    }

    void CharacterRepository::bindValues(const Character &entity, QSqlQuery &query) const
    {

    }
}
