#pragma once

#include <vector>

#include <QSqlDatabase>

namespace Evernus
{
    template<class T>
    class Repository
    {
    public:
        struct NotFoundException : std::exception { };

        Repository(const QSqlDatabase &db);
        virtual ~Repository() = default;

        virtual QString getTableName() const = 0;
        virtual QString getIdColumn() const = 0;

        virtual T populate(const QSqlRecord &record) const = 0;

        QSqlQuery exec(const QString &query) const;
        QSqlQuery prepare(const QString &queryStr) const;
        void store(T &entity) const;

        template<class Id>
        void remove(Id &&id) const;

        std::vector<T> fetchAll() const;

        template<class Id>
        T find(Id &&id) const;

        QSqlDatabase getDatabase() const;

        typename T::IdType getLastInsertId() const;

    private:
        QSqlDatabase mDb;

        void insert(T &entity) const;
        void update(const T &entity) const;

        virtual QStringList getColumns() const = 0;
        virtual void bindValues(const T &entity, QSqlQuery &query) const = 0;

        virtual void preStore(T &entity) const;
        virtual void postStore(T &entity) const;
    };
}

#include "Repository.cpp"
