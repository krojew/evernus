#pragma once

class QSqlDatabase;
class QString;

namespace Evernus
{
    namespace DatabaseUtils
    {
        void createDb(QSqlDatabase &db, const QString &name);
    }
}
