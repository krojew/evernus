#pragma once

#include <vector>

#include <QSqlDatabase>

namespace Evernus
{
    template<class T>
    class Repository
    {
    public:
        Repository(const QSqlDatabase &db);
        virtual ~Repository() = default;

        virtual QString getTableName() const = 0;

        QSqlQuery exec(const QString &query) const;
        void store(const T &entity) const;
        std::vector<T> fetchAll() const;

        QSqlDatabase getDatabase() const;

    private:
        QSqlDatabase mDb;

        virtual T populate(const QSqlQuery &query) const = 0;
        virtual QStringList getColumns() const = 0;
        virtual void bindValues(const T &entity, QSqlQuery &query) const = 0;
    };
}

#include "Repository.cpp"
