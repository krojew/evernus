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

#include <QNetworkAccessManager>
#include <QDateTime>
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
        using Callback = std::function<void (QJsonDocument &&data, const QString &error)>;

        explicit CRESTInterface(QObject *parent = nullptr);
        virtual ~CRESTInterface() = default;

        void fetchBuyMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const;
        void fetchSellMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const;
        void fetchMarketHistory(uint regionId, EveType::IdType typeId, const Callback &callback) const;

        static void setRateLimit(float rate);

    private:
        using RegionOrderUrlMap = QHash<uint, QUrl>;

        static const QString crestUrl;

        static const QString regionsUrlName;
        static const QString itemTypesUrlName;

        static RateLimiter mCRESTLimiter;
        static QHash<QString, QString> mEndpoints;

        mutable QNetworkAccessManager mNetworkManager;

        mutable RegionOrderUrlMap mRegionBuyOrdersUrls, mRegionSellOrdersUrls;
        mutable QHash<QPair<uint, QString>, std::vector<std::function<void (const QUrl &, const QString &)>>>
        mPendingRegionRequests;

        template<class T>
        void getRegionBuyOrdersUrl(uint regionId, T &&continuation) const;

        template<class T>
        void getRegionSellOrdersUrl(uint regionId, T &&continuation) const;

        template<class T>
        void getRegionOrdersUrl(uint regionId,
                                const QString &urlName,
                                RegionOrderUrlMap &map,
                                T &&continuation) const;

        template<class T>
        void getOrders(QUrl regionUrl, EveType::IdType typeId, T &&continuation) const;

        template<class T>
        void asyncGet(const QUrl &url, const QByteArray &accept, T &&continuation) const;

        QNetworkRequest prepareRequest(const QUrl &url, const QByteArray &accept) const;
    };
}
