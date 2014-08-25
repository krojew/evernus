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

#include <QApplication>
#include <QDirIterator>
#include <QSettings>
#include <QDebug>
#include <QFile>

#include "EveCacheFileParser.h"
#include "EveCacheFile.h"
#include "PathSettings.h"

#include "EveCacheManager.h"

namespace Evernus
{
    EveCacheManager::EveCacheManager(const QStringList &machoNetPaths)
        : mMachoNetPaths{machoNetPaths}
    {
    }

    EveCacheManager::EveCacheManager(QStringList &&machoNetPaths)
        : mMachoNetPaths{std::move(machoNetPaths)}
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

    void EveCacheManager::parseMachoNet()
    {

        qDebug() << "Searching paths:" << mMachoNetPaths;

        QSettings settings;
        const auto deleteFiles
            = settings.value(PathSettings::deleteProcessedCacheFilesKey, PathSettings::deleteProcessedCacheFilesDefault).toBool();

        try
        {
            auto counter = 0;

            for (const auto &machoPath : mMachoNetPaths)
            {
                for (const auto &cacheFolder : mCacheFolderFilters)
                {
                    const auto curPath = machoPath + cacheFolder;

                    QDirIterator fileIt{curPath, QStringList{"*.cache"}, QDir::Files | QDir::Readable};
                    while (fileIt.hasNext())
                    {
                        const auto file = fileIt.next();
                        qDebug() << "Parsing file:" << file;

                        try
                        {
                            EveCacheFile cacheFile{file};
                            cacheFile.open();

                            EveCacheFileParser parser{cacheFile};
                            parser.parse();

                            if (deleteFiles)
                                QFile::remove(file);

                            auto &streams = parser.getStreams();

                            mStreams.reserve(mStreams.size() + streams.size());
                            for (auto &stream : streams)
                            {
                                auto &children = stream->getChildren();
                                if (children.empty())
                                    continue;

                                auto &base = children.front();
                                const auto &baseChildren = base->getChildren();
                                if (baseChildren.empty() || baseChildren.front()->getChildren().size() < 2)
                                    continue;

                                const auto id = dynamic_cast<const EveCacheNode::Ident *>(baseChildren.front()->getChildren()[1].get());
                                if (id == nullptr || !mMethodFilters.contains(id->getName()))
                                    continue;

                                mStreams.emplace_back(std::move(base));
                            }

                            ++counter;
                        }
                        catch (const std::exception &e)
                        {
                            qDebug() << e.what();
                        }

                        qApp->processEvents();
                    }
                }
            }

            qDebug() << "Successfully parsed" << counter << "files.";
            qDebug() << "Total streams:" << mStreams.size();
        }
        catch (const std::exception &e)
        {
            qDebug() << e.what();
            throw;
        }
    }

    const std::vector<EveCacheNode::NodePtr> &EveCacheManager::getStreams() const noexcept
    {
        return mStreams;
    }
}
