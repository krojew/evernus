#pragma once

class QSqlDatabase;
class QSqlQuery;
class QString;

namespace Evernus
{
    namespace DatabaseUtils
    {
        void createDb(QSqlDatabase &db, const QString &name);
        void execQuery(QSqlQuery &query);
    }
}
