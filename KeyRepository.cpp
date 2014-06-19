#include <QSqlRecord>
#include <QSqlQuery>

#include "KeyRepository.h"

namespace Evernus
{
    QString KeyRepository::getTableName() const
    {
        return "keys";
    }

    QString KeyRepository::getIdColumn() const
    {
        return "id";
    }

    Key KeyRepository::populate(const QSqlRecord &record) const
    {
        Key key{record.value("id").value<Key::IdType>(), record.value("code").toString()};
        key.setNew(false);

        return key;
    }

    void KeyRepository::create() const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY,
            code TEXT NOT NULL
        ))"}.arg(getTableName()));
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
