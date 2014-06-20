#include <QCoreApplication>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QDir>

#include "DatabaseUtils.h"

namespace Evernus
{
    namespace DatabaseUtils
    {
        void createDb(QSqlDatabase &db, const QString &name)
        {
            if (!db.isValid())
                throw std::runtime_error{QCoreApplication::translate("DatabaseUtils", "Error crating DB object!").toStdString()};

            const auto dbPath =
                QStandardPaths::writableLocation(QStandardPaths::DataLocation) +
                QDir::separator() +
                "db" +
                QDir::separator();

            qDebug() << "DB path: " << dbPath;

            if (!QDir{}.mkpath(dbPath))
                throw std::runtime_error{QCoreApplication::translate("DatabaseUtils", "Error creating DB path!").toStdString()};

            db.setDatabaseName(dbPath + name);
            if (!db.open())
                throw std::runtime_error{QCoreApplication::translate("DatabaseUtils", "Error opening DB!").toStdString()};

            db.exec("PRAGMA foreign_keys = ON");
        }
    }
}
