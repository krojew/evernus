#include <QSqlQuery>

#include "KeyRepository.h"

#include "CharacterRepository.h"

namespace Evernus
{
    QString CharacterRepository::getTableName() const
    {
        return "characters";
    }

    QString CharacterRepository::getIdColumn() const
    {
        return "id";
    }

    Character CharacterRepository::populate(const QSqlRecord &record) const
    {
        Character character;
        character.setKeyId(record.value("key_id").value<Key::IdType>());
        character.setName(record.value("name").toString());
        character.setEnabled(record.value("enabled").toBool());
        character.setNew(false);

        return character;
    }

    void CharacterRepository::create(const KeyRepository &keyRepository) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id BIGINT PRIMARY KEY,
            key_id BITINT NULL REFERENCES %2(id) ON UPDATE SET NULL ON DELETE SET NULL,
            name TEXT NOT NULL,
            enabled TINYINT NOT NULL
        ))"}.arg(getTableName()).arg(keyRepository.getTableName()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_%2_index ON %1(key_id)"}.arg(getTableName()).arg(keyRepository.getTableName()));
    }

    QStringList CharacterRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "key_id"
            << "name"
            << "enabled";
    }

    void CharacterRepository::bindValues(const Character &entity, QSqlQuery &query) const
    {
        const auto keyId = entity.getKeyId();

        query.bindValue(":id", entity.getId());
        query.bindValue(":key_id", (keyId) ? (*keyId) : (QVariant{QVariant::UInt}));
        query.bindValue(":name", entity.getName());
        query.bindValue(":enabled", entity.isEnabled());
    }
}
