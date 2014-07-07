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
#include <unordered_map>
#include <stdexcept>

#include <QStringBuilder>
#include <QSettings>
#include <QFile>
#include <QDir>

#include "PathSettings.h"
#include "PathUtils.h"

#include "MarketLogItemPriceImporterThread.h"

namespace Evernus
{
    void MarketLogItemPriceImporterThread::run()
    {
        const auto logPath = PathUtils::getMarketLogsPath();
        if (logPath.isEmpty())
        {
            emit error(tr("Could not determine market log path!"));
            return;
        }

        const QDir basePath{logPath};
        const auto files = basePath.entryList(QStringList{"*.txt"}, QDir::Files | QDir::Readable);

        ItemPriceList result;

        QSettings settings;
        const auto deleteLogs = settings.value(PathSettings::deleteLogsKey, true).toBool();

        for (const auto &file : files)
        {
            if (isInterruptionRequested())
                break;

            if (file.startsWith("My Orders"))
                continue;

            getItemPrice(logPath % QDir::separator() % file, result, deleteLogs);
        }

        emit finished(result);
    }

    void MarketLogItemPriceImporterThread::getItemPrice(const QString &logPath, ItemPriceList &prices, bool deleteLog)
    {
        std::unordered_map<ItemPrice::LocationIdType, ItemPrice> buy;
        std::unordered_map<ItemPrice::LocationIdType, ItemPrice> sell;

        QFile file{logPath};
        if (!file.open(QIODevice::ReadOnly))
            return;

        file.readLine();

        while (!file.atEnd())
        {
            const QString line = file.readLine();
            const auto values = line.split(',');

            if (values.count() >= 14)
            {
                const auto curValue = values[0].toDouble();
                const auto curLoc = values[10].toULongLong();

                if (values[7] == "True")
                {
                    auto it = buy.find(curLoc);
                    if (it == std::end(buy))
                    {
                        it = buy.emplace(curLoc, ItemPrice::invalidId).first;
                        it->second.setUpdateTime(QDateTime::currentDateTimeUtc());
                        it->second.setLocationId(curLoc);
                        it->second.setType(ItemPrice::Type::Buy);
                        it->second.setTypeId(values[2].toULongLong());
                    }

                    if (curValue > it->second.getValue())
                        it->second.setValue(curValue);
                }
                else
                {
                    auto it = sell.find(curLoc);
                    if (it == std::end(sell))
                    {
                        it = sell.emplace(curLoc, ItemPrice::invalidId).first;
                        it->second.setUpdateTime(QDateTime::currentDateTimeUtc());
                        it->second.setLocationId(curLoc);
                        it->second.setType(ItemPrice::Type::Sell);
                        it->second.setTypeId(values[2].toULongLong());
                        it->second.setValue(std::numeric_limits<double>::max());
                    }

                    if (curValue < it->second.getValue())
                        it->second.setValue(curValue);
                }
            }
        }

        for (auto &price : buy)
            prices.emplace_back(std::move(price.second));
        for (auto &price : sell)
            prices.emplace_back(std::move(price.second));

        if (deleteLog)
            file.remove();
    }
}
