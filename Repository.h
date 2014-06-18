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

        virtual T populate(const QSqlRecord &record) const = 0;

        QSqlQuery exec(const QString &query) const;
        void store(T &entity) const;

        template<class Id>
        void remove(const Id &id) const;

        std::vector<T> fetchAll() const;

        QSqlDatabase getDatabase() const;

    private:
        QSqlDatabase mDb;

        QSqlQuery prepare(const QString &queryStr) const;
        void execQuery(QSqlQuery &query) const;

        void insert(const T &entity) const;
        void update(const T &entity) const;

        virtual QStringList getColumns() const = 0;
        virtual QString getIdColumn() const = 0;
        virtual void bindValues(const T &entity, QSqlQuery &query) const = 0;
    };
}

#include "Repository.cpp"
