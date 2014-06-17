#include <QSqlQuery>

#include "KeyRepository.h"

namespace Evernus
{
    QString KeyRepository::getTableName() const
    {
        return "keys";
    }

    void KeyRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY,
            code TEXT NOT NULL
        ))"}.arg(getTableName()));
    }

    Key KeyRepository::populate(const QSqlQuery &query) const
    {
        return Key{query.value("id").value<Key::IdType>(), query.value("code").toString()};
    }

    QStringList KeyRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "code";
    }

    void KeyRepository::bindValues(const Key &entity, QSqlQuery &query) const
    {
        query.bindValue(":id", entity.getId());
        query.bindValue(":code", entity.getCode());
    }
}
