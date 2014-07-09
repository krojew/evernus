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
#include <QStandardPaths>
#include <QStringBuilder>
#include <QSettings>
#include <QDebug>
#include <QDir>

#include "PathSettings.h"
#include "ItemPrice.h"

#include "CacheItemPriceImporter.h"

namespace Evernus
{
    void CacheItemPriceImporter::fetchItemPrices(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit itemPricesChanged(std::vector<ItemPrice>{});
            return;
        }

        const auto cachePaths = getEveCachePaths();
        if (cachePaths.isEmpty())
        {
            emit error(tr("Couldn't determine Eve cache path."));
            return;
        }
    }

    QStringList CacheItemPriceImporter::getEveCachePaths()
    {
        QSettings settings;

        const auto bulkDataPathSegment = "bulkdata/";
        const auto machoNetPathSegment = "MachoNet/";
        const auto tranquilityIpPathSegment = "87.237.38.200/";

#ifdef Q_OS_WIN
        qDebug() << "Looking for eve cache path...";

        auto basePath = settings.value(PathSettings::evePathKey).toString().toLower();
        qDebug() << "Eve path:" << basePath;

        if (basePath.isEmpty())
            return QStringList{};

        const QString tranquilityPathSegment = "_tranquility";
        const QString cachePathSegment = "cache";

        basePath.replace(":", QString{}).replace("\\", "_").replace(" ", "_").replace("/", "_");
        basePath += tranquilityPathSegment % "/" % cachePathSegment % "/";

        qDebug() << "Combined base path:" << basePath;

        const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) % "/CCP/EVE/";
        qDebug() << "App data path:" << appDataPath;

        basePath.prepend(appDataPath);
#else
        auto basePath = settings.value(PathSettings::eveCachePathKey).toString();
        qDebug() << "Eve path:" << basePath;

        if (basePath.isEmpty())
            return QStringList{};

        basePath += "/";

#endif
        const QString bulkDataPath = basePath % bulkDataPathSegment;
        const QString machoNetPath = basePath % machoNetPathSegment % tranquilityIpPathSegment;

        return QStringList{} << bulkDataPath << machoNetPath;
    }
}
