#include <QSqlQuery>

#include "KeyRepository.h"

#include "CharacterRepository.h"

namespace Evernus
{
    void CharacterRepository::create(const KeyRepository &keyRepository) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS characters (
            id BIGINT PRIMARY KEY,
            key_id BITINT NOT NULL REFERENCES %1(id),
            name TEXT NOT NULL,
            enabled TINYINT NOT NULL
        ))"}.arg(keyRepository.getTableName()));
    }
}
