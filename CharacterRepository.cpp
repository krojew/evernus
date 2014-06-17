#include <QSqlQuery>

#include "CharacterRepository.h"

namespace Evernus
{
    void CharacterRepository::create() const
    {
        exec(R"(CREATE TABLE IF NOT EXISTS characters (
            id BIGINT PRIMARY KEY,
            name TEXT NOT NULL,
            enabled TINYINT NOT NULL
        ))");
    }
}
