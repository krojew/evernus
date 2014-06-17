#include <stdexcept>

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include "Repository.h"

namespace Evernus
{
    Repository::Repository(const QSqlDatabase &db)
        : mDb{db}
    {
    }

    QSqlQuery Repository::exec(const QString &query) const
    {
        qDebug() << "DB query: " << query;

        auto result = mDb.exec(query);
        const auto error = mDb.lastError();

        if (error.isValid())
        {
            qCritical() << error.text();
            throw std::runtime_error{error.text().toStdString()};
        }

        return result;
    }
}
