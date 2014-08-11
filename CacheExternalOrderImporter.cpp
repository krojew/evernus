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

#include "EveCacheManager.h"
#include "ExternalOrder.h"
#include "PathSettings.h"

#include "CacheExternalOrderImporter.h"

namespace Evernus
{
    void CacheExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
            return;
        }

        const auto cachePath = getEveCachePath();
        if (cachePath.isEmpty())
        {
            emit error(tr("Couldn't determine Eve cache path."));
            return;
        }

        if (!QDir{cachePath}.exists())
        {
            emit error(tr("Eve cache path doesn't exist."));
            return;
        }

        EveCacheManager manager{cachePath};
        manager.addCacheFolderFilter("CachedMethodCalls");
        manager.addMethodFilter("GetOrders");

        try
        {
            manager.parseMachoNet();
        }
        catch (const std::exception &e)
        {
            emit error(e.what());
        }
    }

    QString CacheExternalOrderImporter::getEveCachePath()
    {
        QSettings settings;

        const auto machoNetPathSegment = "MachoNet/";
        const auto tranquilityIpPathSegment = "87.237.38.200/";

#ifdef Q_OS_WIN
        qDebug() << "Looking for eve cache path...";

        auto basePath = settings.value(PathSettings::evePathKey).toString();
        qDebug() << "Eve path:" << basePath;

        if (basePath.isEmpty())
            return QString{};

        const QString tranquilityPathSegment = "_tranquility";
        const QString cachePathSegment = "cache";

        basePath.remove(":").replace("\\", "_").replace(" ", "_").replace("/", "_");
        basePath += tranquilityPathSegment % "/" % cachePathSegment % "/";

        qDebug() << "Combined base path:" << basePath;

        const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) % "/CCP/EVE/";
        qDebug() << "App data path:" << appDataPath;

        basePath.prepend(appDataPath);
#else
        auto basePath = settings.value(PathSettings::eveCachePathKey).toString();
        qDebug() << "Eve path:" << basePath;

        if (basePath.isEmpty())
            return QString{};

        basePath += "/";
#endif
        return basePath % machoNetPathSegment % tranquilityIpPathSegment;
    }
}
