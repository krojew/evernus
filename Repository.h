/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
        void batchStore(const std::vector<T> &entities) const;

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
        virtual void bindPositionalValues(const T &entity, QSqlQuery &query) const = 0;

        virtual void preStore(T &entity) const;
        virtual void postStore(T &entity) const;
    };
}

#include "Repository.cpp"
