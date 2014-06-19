#include <stdexcept>

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace Evernus
{
    template<class T>
    Repository<T>::Repository(const QSqlDatabase &db)
        : mDb{db}
    {
    }

    template<class T>
    QSqlQuery Repository<T>::exec(const QString &query) const
    {
        auto result = mDb.exec(query);
        const auto error = mDb.lastError();

        if (error.isValid())
        {
            qCritical() << error.text();
            throw std::runtime_error{error.text().toStdString()};
        }

        return result;
    }

    template<class T>
    QSqlQuery Repository<T>::prepare(const QString &queryStr) const
    {
        QSqlQuery query{mDb};
        if (!query.prepare(queryStr))
        {
            qCritical() << "Error preparing statement!";
            throw std::runtime_error{"Error preparing statement!"};
        }

        return query;
    }

    template<class T>
    void Repository<T>::store(T &entity) const
    {
        if (entity.isNew())
            insert(entity);
        else
            update(entity);

        entity.updateOriginalId();
    }

    template<class T>
    template<class Id>
    void Repository<T>::remove(const Id &id) const
    {
        auto query = prepare(QString{"DELETE FROM %1 WHERE %2 = :id"}.arg(getTableName()).arg(getIdColumn()));
        query.bindValue(":id", id);
        execQuery(query);
    }

    template<class T>
    std::vector<T> Repository<T>::fetchAll() const
    {
        std::vector<T> out;

        auto result = exec(QString{"SELECT * FROM %1"}.arg(getTableName()));
        const auto size = result.size();
        if (size != -1)
            out.reserve(size);

        while (result.next())
            out.emplace_back(populate(result.record()));

        return out;
    }

    template<class T>
    QSqlDatabase Repository<T>::getDatabase() const
    {
        return mDb;
    }

    template<class T>
    void Repository<T>::execQuery(QSqlQuery &query) const
    {
        if (!query.exec())
        {
            auto error = query.lastError().text();

            qCritical() << error;
            throw std::runtime_error{error.toStdString()};
        }
    }

    template<class T>
    void Repository<T>::insert(const T &entity) const
    {
        const auto columns = getColumns();

        QStringList prefixedColumns;
        for (const auto &column : columns)
            prefixedColumns << ":" + column;

        const auto queryStr = QString{"REPLACE INTO %1 (%2) VALUES (%3)"}
            .arg(getTableName())
            .arg(columns.join(", "))
            .arg(prefixedColumns.join(", "));

        auto query = prepare(queryStr);
        bindValues(entity, query);
        execQuery(query);
    }

    template<class T>
    void Repository<T>::update(const T &entity) const
    {
        const auto columns = getColumns();

        QStringList updateList;
        for (const auto &column : columns)
            updateList << QString("%1 = :%1").arg(column);

        const auto queryStr = QString{"UPDATE %1 SET %2 WHERE %3 = :id_for_update"}
            .arg(getTableName())
            .arg(updateList.join(", "))
            .arg(getIdColumn());

        auto query = prepare(queryStr);
        query.bindValue(":id_for_update", entity.getOriginalId());

        bindValues(entity, query);
        execQuery(query);
    }
}
