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
#include <QStringBuilder>
#include <QFileInfo>
#include <QSettings>
#include <QFile>
#include <QDir>

#include "PathSettings.h"
#include "PathUtils.h"

#include "MarketLogExternalOrderImporterThread.h"

namespace Evernus
{
    void MarketLogExternalOrderImporterThread::run()
    {
        const auto logPath = PathUtils::getMarketLogsPath();
        if (logPath.isEmpty())
        {
            emit error(tr("Could not determine market log path!"));
            return;
        }

        const QDir basePath{logPath};
        const auto files = basePath.entryList(QStringList{"*.txt"}, QDir::Files | QDir::Readable);

        ExternalOrderList result;

        QSettings settings;
        const auto deleteLogs = settings.value(PathSettings::deleteLogsKey, true).toBool();

        for (const auto &file : files)
        {
            if (isInterruptionRequested())
                break;

            if (file.startsWith("My Orders"))
                continue;

            getExternalOrder(logPath % "/" % file, result, deleteLogs);
        }

        emit finished(result);
    }

    void MarketLogExternalOrderImporterThread::getExternalOrder(const QString &logPath, ExternalOrderList &orders, bool deleteLog)
    {
        QFile file{logPath};
        if (!file.open(QIODevice::ReadOnly))
            return;

        file.readLine();

        QFileInfo info{file};
        const auto priceTime = info.created().toUTC();

        const auto logColumns = 14;

        const auto priceColumn = 0;
        const auto typeColumn = 2;
        const auto rangeColumn = 3;
        const auto idColumn = 4;
        const auto bidColumn = 7;
        const auto stationColumn = 10;
        const auto regionColumn = 11;
        const auto systemColumn = 12;

        while (!file.atEnd())
        {
            const QString line = file.readLine();
            const auto values = line.split(',');

            if (values.count() >= logColumns)
            {
                ExternalOrder price;
                price.setId(values[idColumn].toUInt());
                price.setUpdateTime(priceTime);
                price.setLocationId(values[stationColumn].toULongLong());
                price.setSolarSystemId(values[systemColumn].toUInt());
                price.setRegionId(values[regionColumn].toUInt());
                price.setRange(values[rangeColumn].toShort());
                price.setType((values[bidColumn] == "True") ? (ExternalOrder::Type::Buy) : (ExternalOrder::Type::Sell));
                price.setTypeId(values[typeColumn].toULongLong());
                price.setValue(values[priceColumn].toDouble());

                orders.emplace_back(std::move(price));
            }
        }

        if (deleteLog)
            file.remove();
    }
}
