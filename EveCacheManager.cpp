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

    void EveCacheManager::parseMachoNet()
    {
        auto max = 0u;

        qDebug() << "Searching path:" << mMachoNetPath;

        QDirIterator dirIt{mMachoNetPath};
        while (dirIt.hasNext())
        {
            dirIt.next();

            auto ok = false;
            const auto cur = dirIt.fileName().toUInt(&ok);

            if (ok && cur > max)
                max = cur;
        }

        if (max == 0)
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheManager", "No cache files found!")};

        qDebug() << "Cache version:" << max;

        const QString basePath = mMachoNetPath % "/" % QString::number(max) % "/";

        try
        {
            auto counter = 0;

            for (const auto &cacheFolder : mCacheFolderFilters)
            {
                const auto curPath = basePath + cacheFolder;

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
