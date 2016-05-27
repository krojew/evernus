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

#include <unordered_set>
#include <functional>
#include <vector>
#include <chrono>
#include <map>

#include <QNetworkAccessManager>
#include <QDateTime>
#include <QTimer>
#include <QHash>
#include <QUrl>

#include "RateLimiter.h"
#include "EveType.h"

class QJsonDocument;

namespace Evernus
{
    class CRESTInterface
        : public QObject
    {
        Q_OBJECT

    public:
        using JsonCallback = std::function<void (QJsonDocument &&data, const QString &error)>;
        using PaginatedCallback = std::function<void (QJsonDocument &&data, bool atEnd, const QString &error)>;

        using EndpointMap = QHash<QString, QString>;

        static const QString crestUrl;

        explicit CRESTInterface(QObject *parent = nullptr);
        virtual ~CRESTInterface() = default;

        void fetchMarketOrders(uint regionId, EveType::IdType typeId, const JsonCallback &callback) const;
        void fetchMarketOrders(uint regionId, const PaginatedCallback &callback) const;
        void fetchMarketHistory(uint regionId, EveType::IdType typeId, const JsonCallback &callback) const;

        void setEndpoints(EndpointMap endpoints);

        static void setRateLimit(float rate);

    private slots:
        void processSslErrors(const QList<QSslError> &errors);
        void processPendingRequests();

    private:
        using RegionUrlMap = QHash<uint, QUrl>;

        static const QString regionsUrlName;
        static const QString itemTypesUrlName;

        static RateLimiter mCRESTLimiter;
        static QTimer mRequestTimer;

        mutable QNetworkAccessManager mNetworkManager;
        EndpointMap mEndpoints;

        mutable RegionUrlMap mRegionOrdersUrls, mRegionMarketUrls;
        mutable QHash<QPair<uint, QString>, std::vector<std::function<void (const QUrl &, const QString &)>>>
        mPendingRegionRequests;

        mutable std::multimap<std::chrono::steady_clock::time_point, std::function<void ()>> mPendingRequests;

        template<class T>
        void getRegionOrdersUrl(uint regionId, T &&continuation) const;
        template<class T>
        void getRegionMarketUrl(uint regionId, T &&continuation) const;

        template<class T>
        void getRegionUrl(uint regionId, const QString &urlName, T &&continuation) const;

        template<class T>
        void getOrders(QUrl regionUrl, EveType::IdType typeId, T &&continuation) const;
        template<class T>
        void getOrders(QUrl regionUrl, T &&continuation) const;

        void fetchPaginatedOrders(const PaginatedCallback &callback, const QUrl &url) const;

        template<class T>
        void asyncGet(const QUrl &url, const QByteArray &accept, T &&continuation) const;

        QNetworkRequest prepareRequest(const QUrl &url, const QByteArray &accept) const;
    };
}
