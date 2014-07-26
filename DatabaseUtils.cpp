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
#include <QStringBuilder>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
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

            const QString dbPath =
                QStandardPaths::writableLocation(QStandardPaths::DataLocation) %
                "/db/";

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
    }
}
