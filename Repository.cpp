#include <stdexcept>

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include "Repository.h"

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
        qDebug() << "DB query: " << query;

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
    void Repository<T>::store(const T &entity) const
    {
        const auto columns = getColumns();

        QStringList prefixedColumns;
        for (const auto &column : columns)
            prefixedColumns << ":" + column;

        const auto queryStr = QString{"REPLACE INTO %1 (%2) VALUES (%3)"}
            .arg(getTableName())
            .arg(columns.join(", "))
            .arg(prefixedColumns.join(", "));

        qDebug() << "DB query: " << queryStr;

        QSqlQuery query{mDb};
        if (!query.prepare(queryStr))
        {
            qCritical() << "Error preparing statement!";
            throw std::runtime_error{"Error preparing statement!"};
        }

        bindValues(entity, query);

        if (!query.exec())
        {
            auto error = query.lastError().text();

            qCritical() << error;
            throw std::runtime_error{error.toStdString()};
        }
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
            out.emplace_back(populate(result));

        return out;
    }

    template<class T>
    QSqlDatabase Repository<T>::getDatabase() const
    {
        return mDb;
    }
}
