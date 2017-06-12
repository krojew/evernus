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
#pragma once

#include <unordered_map>
#include <exception>
#include <memory>
#include <list>

#include <QAbstractNetworkCache>
#include <QDateTime>
#include <QDir>

namespace Evernus
{
    class APIResponseCache
        : public QAbstractNetworkCache
    {
        Q_OBJECT

    public:
        explicit APIResponseCache(QObject *parent = nullptr);
        virtual ~APIResponseCache() = default;

        virtual qint64 cacheSize() const override;
        virtual QIODevice *data(const QUrl &url) override;
        virtual void insert(QIODevice *device) override;
        virtual QNetworkCacheMetaData metaData(const QUrl &url) override;
        virtual QIODevice *prepare(const QNetworkCacheMetaData &metaData) override;
        virtual bool remove(const QUrl &url) override;
        virtual void updateMetaData(const QNetworkCacheMetaData &metaData) override;

        virtual void clear() override;

    private:
        static const QString cacheDir;

        struct ErrorReadingCacheException : std::exception { };

        struct CacheItem
        {
            QByteArray mData;
            QDateTime mCacheTime;
            QNetworkCacheMetaData mMetaData;
        };

        struct UrlHash
        {
            size_t operator ()(const QUrl &url) const;
        };

        QDir mCacheDir;

        std::unordered_map<QUrl, std::unique_ptr<CacheItem>, UrlHash> mData;
        std::unordered_map<QIODevice *, std::unique_ptr<CacheItem>> mInsertingDataMap;
        std::list<std::unique_ptr<QIODevice>> mInsertingData;

        CacheItem *readFile(const QUrl &url);
        void writeFile(const CacheItem &item);
        bool removeFile(const QUrl &url);
        bool removeFileWithNonEmptyParent(const QString &file);

        QString getExistingCacheFileName(const QUrl &url) const;
        QString getNewCacheFileName(const QUrl &url) const;
    };
}
