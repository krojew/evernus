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
#include <stdexcept>

#include <QCoreApplication>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QDir>

#include "DatabaseUtils.h"

namespace Evernus
{
    namespace DatabaseUtils
    {
        QString getDbPath()
        {
            return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/db/";
        }

        void createDb(QSqlDatabase &db, const QString &name)
        {
            if (!db.isValid())
                throw std::runtime_error{QCoreApplication::translate("DatabaseUtils", "Error creating DB object!").toStdString()};

            const auto dbPath = getDbPath();

            qDebug() << "DB path: " << dbPath;

            if (!QDir{}.mkpath(dbPath))
                throw std::runtime_error{QCoreApplication::translate("DatabaseUtils", "Error creating DB path!").toStdString()};

            db.setDatabaseName(dbPath + name);
            if (!db.open())
                throw std::runtime_error{QCoreApplication::translate("DatabaseUtils", "Error opening DB!").toStdString()};

            db.exec("PRAGMA foreign_keys = ON");
        }

        void execQuery(QSqlQuery &query)
        {
            if (!query.exec())
            {
                const auto error = QString{"%1: %2"}.arg(query.lastError().text()).arg(query.lastQuery());

                qCritical() << error;
                throw std::runtime_error{error.toStdString()};
            }
        }

        QString backupDatabase(const QSqlDatabase &db)
        {
            return backupDatabase(db.databaseName());
        }

        QString backupDatabase(const QString &dbPath)
        {
            const auto dbBak = dbPath + ".bak";

            QFile::remove(dbBak);
            QFile::copy(dbPath, dbBak);

            return dbBak;
        }
    }
}
