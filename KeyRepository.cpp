#include <QSqlQuery>

#include "KeyRepository.h"

namespace Evernus
{
    void KeyRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id BIGINT PRIMARY KEY,
            code TEXT NOT NULL
        ))"}.arg(getTableName()));
    }

    QString KeyRepository::getTableName() const
    {
        return "keys";
    }
}
