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

#include <boost/throw_exception.hpp>

#include <QCoreApplication>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QtDebug>
#include <QFile>
#include <QDir>

#include "DatabaseUtils.h"

namespace Evernus
{
    namespace DatabaseUtils
    {
        QString getDbPath()
        {
            return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/db/";
        }

        void createDb(QSqlDatabase &db, const QString &name)
        {
            if (!db.isValid())
                BOOST_THROW_EXCEPTION(std::runtime_error{QCoreApplication::translate("DatabaseUtils", "Error creating DB object!").toStdString()});

            const auto dbPath = getDbPath();

            qDebug() << "DB path: " << dbPath;

            if (!QDir{}.mkpath(dbPath))
                BOOST_THROW_EXCEPTION(std::runtime_error{QCoreApplication::translate("DatabaseUtils", "Error creating DB path!").toStdString()});

            db.setDatabaseName(dbPath + name);
            if (!db.open())
                BOOST_THROW_EXCEPTION(std::runtime_error{QCoreApplication::translate("DatabaseUtils", "Error opening DB!").toStdString()});

            db.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        }

        void execQuery(QSqlQuery &query)
        {
            qDebug() << "SQL:" << query.lastQuery();
            if (!query.exec())
            {
                const auto error = query.lastError().text();

                qCritical() << error;
                BOOST_THROW_EXCEPTION(std::runtime_error{error.toStdString()});
            }
        }

        QString backupDatabase(const QSqlDatabase &db)
        {
            return backupDatabase(db.databaseName());
        }

        QString backupDatabase(const QString &dbPath)
        {
            const auto dbBak = dbPath + QStringLiteral(".bak");

            QFile::remove(dbBak);
            QFile::copy(dbPath, dbBak);

            return dbBak;
        }
    }
}
