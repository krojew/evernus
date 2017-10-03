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
#include <QDataStream>
#include <QSettings>
#include <QFile>
#include <QDir>

#include "NetworkSettings.h"
#include "ImportSettings.h"
#include "ESIInterface.h"

#include "ESIInterfaceManager.h"

namespace Evernus
{
    ESIInterfaceManager::ESIInterfaceManager(QObject *parent)
        : QObject{parent}
    {
        readCitadelAccessCache();
        createInterfaces();
        connectInterfaces();
    }

    ESIInterfaceManager::~ESIInterfaceManager()
    {
        try
        {
            writeCitadelAccessCache();
        }
        catch (...)
        {
        }
    }

    void ESIInterfaceManager::handleNewPreferences()
    {
        mInterfaces.clear();

        createInterfaces();
        connectInterfaces();
    }

    const ESIInterface &ESIInterfaceManager::selectNextInterface()
    {
        const auto &interface = *mInterfaces[mCurrentInterface];
        mCurrentInterface = (mCurrentInterface + 1) % mInterfaces.size();

        return interface;
    }

    void ESIInterfaceManager::connectInterfaces()
    {
        for (const auto &interface : mInterfaces)
        {
            connect(interface.get(), &ESIInterface::tokenRequested, this, &ESIInterfaceManager::tokenRequested, Qt::QueuedConnection);
            connect(this, &ESIInterfaceManager::acquiredToken, interface.get(), &ESIInterface::updateTokenAndContinue);
            connect(this, &ESIInterfaceManager::tokenError, interface.get(), &ESIInterface::handleTokenError);
        }
    }

    void ESIInterfaceManager::createInterfaces()
    {
        QSettings settings;

        // IO bound
        const auto maxInterfaces = std::max(
            settings.value(NetworkSettings::maxESIThreadsKey, NetworkSettings::maxESIThreadsDefault).toUInt(),
            1u
        );

        for (auto i = 0u; i < maxInterfaces; ++i)
            mInterfaces.emplace_back(new ESIInterface{mCitadelAccessCache, mErrorLimiter});
    }

    void ESIInterfaceManager::readCitadelAccessCache()
    {
        QFile cacheFile{getCachePath()};
        QDataStream stream{&cacheFile};

        if (cacheFile.open(QIODevice::ReadOnly))
        {
            stream >> mCitadelAccessCache;

            QSettings settings;
            mCitadelAccessCache.clearIfObsolete(QDateTime::currentDateTime().addDays(
                -settings.value(ImportSettings::maxCitadelAccessAgeKey, ImportSettings::maxCitadelAccessAgeDefault).toInt()
            ));
        }
    }

    void ESIInterfaceManager::writeCitadelAccessCache()
    {
        QFile cacheFile{getCachePath()};
        QDataStream stream{&cacheFile};

        if (cacheFile.open(QIODevice::WriteOnly))
            stream << mCitadelAccessCache;
    }

    QString ESIInterfaceManager::getCachePath()
    {
        return QDir{QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/data")}.filePath(QStringLiteral("citadel_access"));
    }
}
