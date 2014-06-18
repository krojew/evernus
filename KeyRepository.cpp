#include <QSqlRecord>
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

    Key KeyRepository::populate(const QSqlRecord &record) const
    {
        Key key{record.value("id").value<Key::IdType>(), record.value("code").toString()};
        key.setNew(false);

        return key;
    }

    QStringList KeyRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "code";
    }

    QString KeyRepository::getIdColumn() const
    {
        return "id";
    }

    void KeyRepository::bindValues(const Key &entity, QSqlQuery &query) const
    {
        query.bindValue(":id", entity.getId());
        query.bindValue(":code", entity.getCode());
    }
}
