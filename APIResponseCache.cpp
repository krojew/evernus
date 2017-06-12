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
#include <QStandardPaths>
#include <QDataStream>
#include <QSaveFile>
#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QHash>
#include <QUuid>

#include "APIUtils.h"

#include "APIResponseCache.h"

namespace Evernus
{
    const QString APIResponseCache::cacheDir = "";

    size_t APIResponseCache::UrlHash::operator ()(const QUrl &url) const
    {
        return qHash(url);
    }

    APIResponseCache::APIResponseCache(QObject *parent)
        : QAbstractNetworkCache(parent)
    {
        qDebug() << "Creating response cache path.";

        const QString path =
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation) %
            "/api";

        qDebug() << "Cache path:" << path;

        mCacheDir.setPath(path);

        if (!mCacheDir.mkpath("."))
            throw std::runtime_error{tr("Cannot create cache path.").toStdString()};
    }

    qint64 APIResponseCache::cacheSize() const
    {
        const auto dirs = mCacheDir.entryList(QDir::Dirs);
        qint64 out = 0;

        for (const auto &dir : dirs)
        {
            const auto files = QDir{mCacheDir.filePath(dir)}.entryInfoList(QDir::Files | QDir::Readable);
            for (const auto &file : files)
                out += file.size();
        }

        return out;
    }

    QIODevice *APIResponseCache::data(const QUrl &url)
    {
        qDebug() << "Retrieving data for" << url;

        if (!url.isValid())
            return nullptr;

        const auto it = mData.find(url);
        if (it != std::end(mData))
        {
            qDebug() << "Found in cache.";

            if (it->second->mCacheTime < QDateTime::currentDateTimeUtc())
            {
                qDebug() << "Data too old.";
                mData.erase(it);

                removeFile(url);
            }
            else
            {
                auto buffer = std::make_unique<QBuffer>();
                buffer->setData(it->second->mData);
                buffer->open(QIODevice::ReadOnly);

                return buffer.release();
            }
        }

        try
        {
            const auto item = readFile(url);

            auto buffer = std::make_unique<QBuffer>();
            buffer->setData(item->mData);
            buffer->open(QIODevice::ReadOnly);

            return buffer.release();
        }
        catch (const ErrorReadingCacheException &)
        {
            return nullptr;
        }
    }

    void APIResponseCache::insert(QIODevice *device)
    {
        const auto mIt = mInsertingDataMap.find(device);
        if (mIt == std::end(mInsertingDataMap))
        {
            qWarning() << "Insert called on an unknown device";
            return;
        }

        auto item = std::move(mIt->second);
        mInsertingDataMap.erase(mIt);

        const auto url = item->mMetaData.url();
        qDebug() << "Saving cache:" << url;

        const auto it = std::find_if(std::begin(mInsertingData), std::end(mInsertingData), [device](const auto &entry) {
            return entry.get() == device;
        });
        Q_ASSERT(it != std::end(mInsertingData));

        mInsertingData.erase(it);

        item->mCacheTime = APIUtils::getCachedUntil(item->mData);
        item->mMetaData.setExpirationDate(item->mCacheTime);

        qDebug() << "Cache time:" << item->mCacheTime;

        if (!removeFile(url))
        {
            qWarning() << "Couldn't remove file.";
            mData.erase(url);
            return;
        }

        writeFile(*item);

        mData[url] = std::move(item);
    }

    QNetworkCacheMetaData APIResponseCache::metaData(const QUrl &url)
    {
        if (!url.isValid())
            return QNetworkCacheMetaData{};

        const auto it = mData.find(url);
        if (it != std::end(mData))
            return it->second->mMetaData;

        try
        {
            return readFile(url)->mMetaData;
        }
        catch (const ErrorReadingCacheException &)
        {
            return QNetworkCacheMetaData{};
        }
    }

    QIODevice *APIResponseCache::prepare(const QNetworkCacheMetaData &metaData)
    {
        qDebug() << "Preparing device for" << metaData.url();

        auto item = std::make_unique<CacheItem>();
        auto device = std::make_unique<QBuffer>(&item->mData);
        auto devicePtr = device.get();

        item->mMetaData = metaData;
        device->open(QIODevice::ReadWrite);

        mInsertingDataMap[device.get()] = std::move(item);
        mInsertingData.emplace_back(std::move(device));

        return devicePtr;
    }

    bool APIResponseCache::remove(const QUrl &url)
    {
        const auto it = std::find_if(std::begin(mInsertingDataMap), std::end(mInsertingDataMap), [url](const auto &entry) {
            return entry.second->mMetaData.url() == url;
        });

        if (it != std::end(mInsertingDataMap))
        {
            const auto dIt = std::find_if(std::begin(mInsertingData), std::end(mInsertingData), [&it](const auto &entry) {
                return entry.get() == it->first;
            });

            mInsertingDataMap.erase(it);
            Q_ASSERT(dIt != std::end(mInsertingData));

            mInsertingData.erase(dIt);
            return true;
        }

        removeFile(url);
        mData.erase(url);

        return true;
    }

    void APIResponseCache::updateMetaData(const QNetworkCacheMetaData &metaData)
    {
        const auto url = metaData.url();
        const auto it = mData.find(url);
        if (it != std::end(mData))
        {
            it->second->mMetaData = metaData;
            it->second->mMetaData.setExpirationDate(it->second->mCacheTime);
        }

        const auto fileName = getExistingCacheFileName(url);
        if (!fileName.isEmpty())
        {
            mCacheDir.remove(fileName);

            try
            {
                auto item = readFile(url);
                item->mMetaData = metaData;
                item->mMetaData.setExpirationDate(item->mCacheTime);

                writeFile(*item);
            }
            catch (const ErrorReadingCacheException &)
            {
            }
        }
    }

    void APIResponseCache::clear()
    {
        qDebug() << "Clearing data.";
        mData.clear();
        mCacheDir.removeRecursively();
        mCacheDir.mkpath(".");
    }

    APIResponseCache::CacheItem *APIResponseCache::readFile(const QUrl &url)
    {
        const auto fileName = getExistingCacheFileName(url);

        qDebug() << "Reading cache file:" << fileName;

        QFile file{mCacheDir.filePath(fileName)};
        if (!file.open(QIODevice::ReadOnly))
        {
            qDebug() << "Error opening file.";
            throw ErrorReadingCacheException{};
        }

        QDataStream stream{&file};

        auto item = std::make_unique<CacheItem>();
        auto itemPtr = item.get();

        stream >> item->mCacheTime;

        qDebug() << "Cache time:" << item->mCacheTime;

        if (!item->mCacheTime.isValid() || item->mCacheTime < QDateTime::currentDateTimeUtc())
        {
            removeFileWithNonEmptyParent(mCacheDir.filePath(fileName));
            throw ErrorReadingCacheException{};
        }

        stream >> item->mMetaData >> item->mData;

        mData[url] = std::move(item);

        return itemPtr;
    }

    void APIResponseCache::writeFile(const CacheItem &item)
    {
        const auto url = item.mMetaData.url();
        const auto fileName = getNewCacheFileName(url);

        qDebug() << "Saving file:" << fileName;

        QSaveFile file{mCacheDir.filePath(fileName)};
        if (!file.open(QIODevice::WriteOnly))
        {
            qWarning() << "Couldn't open save file.";
            mData.erase(url);
            return;
        }

        QDataStream stream{&file};

        stream << item.mCacheTime << item.mMetaData << item.mData;
        if (!file.commit())
        {
            qWarning() << "Couldn't commit save file.";
            mData.erase(url);
        }
    }

    bool APIResponseCache::removeFile(const QUrl &url)
    {
        const auto fileName = getExistingCacheFileName(url);
        return removeFileWithNonEmptyParent(fileName);
    }

    bool APIResponseCache::removeFileWithNonEmptyParent(const QString &file)
    {
        if (file.isEmpty())
            return true;

        if (!mCacheDir.remove(file))
            return false;

        QDir dir{file};
        dir.cdUp();

        if (dir.count() == 0)
            mCacheDir.rmdir(dir.dirName());

        return true;
    }

    QString APIResponseCache::getExistingCacheFileName(const QUrl &url) const
    {
        const auto hash = qHash(url);

        QDir hashDir{mCacheDir};
        if (!hashDir.cd(QString::number(hash)))
            return QString{};

        const auto files = hashDir.entryList(QDir::Files | QDir::Readable);

        qDebug() << "Looking for cache file in" << hashDir.absolutePath();

        for (const auto &file : files)
        {
            QFile cacheFile{hashDir.filePath(file)};
            if (!cacheFile.open(QIODevice::ReadOnly))
                continue;

            QDateTime tempDt;
            QNetworkCacheMetaData tempMeta;

            QDataStream stream{&cacheFile};
            stream >> tempDt >> tempMeta;

            if (tempMeta.url() == url)
                return hashDir.dirName() % "/" % file;
        }

        return QString{};
    }

    QString APIResponseCache::getNewCacheFileName(const QUrl &url) const
    {
        const auto existing = getExistingCacheFileName(url);
        if (!existing.isEmpty())
            return existing;

        const auto hash = QString::number(qHash(url));
        mCacheDir.mkdir(hash);

        return QString{"%1/%2"}.arg(hash).arg(QUuid::createUuid().toString());
    }
}
