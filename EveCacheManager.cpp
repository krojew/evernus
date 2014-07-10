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
#include <QDirIterator>
#include <QDebug>
#include <QDir>

#include "EveCacheFileParser.h"
#include "EveCacheFile.h"

#include "EveCacheManager.h"

namespace Evernus
{
    EveCacheManager::EveCacheManager(const QString &machoNetPath)
        : mMachoNetPath{machoNetPath}
    {
    }

    EveCacheManager::EveCacheManager(QString &&machoNetPath)
        : mMachoNetPath{std::move(machoNetPath)}
    {
    }

    void EveCacheManager::addCacheFolderFilter(const QString &name)
    {
        mCacheFolderFilters << name;
    }

    void EveCacheManager::addCacheFolderFilter(QString &&name)
    {
        mCacheFolderFilters << std::move(name);
    }

    void EveCacheManager::addMethodFilter(const QString &name)
    {
        mMethodFilters << name;
    }

    void EveCacheManager::addMethodFilter(QString &&name)
    {
        mMethodFilters << std::move(name);
    }

    void EveCacheManager::parseMachoNet() const
    {
        auto max = 0u;

        qDebug() << "Searching path:" << mMachoNetPath;

        QDirIterator dirIt{mMachoNetPath};
        while (dirIt.hasNext())
        {
            auto ok = false;
            const auto cur = dirIt.next().toUInt(&ok);

            if (ok && cur > max)
                max = cur;
        }

        if (max == 0)
            return;

        qDebug() << "Cache version:" << max;

        const QString basePath = mMachoNetPath % "/" % QString::number(max);

        QDirIterator fileIt{basePath, QStringList{"*.cache"}, QDir::Files | QDir::Readable};
        try
        {
            while (fileIt.hasNext())
            {
                const QString file = basePath % "/" % fileIt.next();
                qDebug() << "Parsing file:" << file;

                EveCacheFile cacheFile{file};
                cacheFile.open();

                EveCacheFileParser parser{cacheFile};
                parser.parse();
            }
        }
        catch (const std::exception &e)
        {
            qDebug() << e.what();
            throw;
        }
    }
}
