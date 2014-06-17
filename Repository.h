#pragma once

#include <QSqlDatabase>

namespace Evernus
{
    class Repository
    {
    public:
        Repository(const QSqlDatabase &db);
        virtual ~Repository() = default;

        QSqlQuery exec(const QString &query) const;

    private:
        QSqlDatabase mDb;
    };
}
