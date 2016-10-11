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
#include "Character.h"
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
        using ErrorCallback = std::function<void (const QString &error)>;

        using EndpointMap = QHash<QString, QString>;

        static const QString crestUrl;

        explicit CRESTInterface(QObject *parent = nullptr);
        virtual ~CRESTInterface() = default;

        void fetchMarketOrders(uint regionId, EveType::IdType typeId, const JsonCallback &callback) const;
        void fetchMarketOrders(uint regionId, const PaginatedCallback &callback) const;
        void fetchMarketHistory(uint regionId, EveType::IdType typeId, const JsonCallback &callback) const;

        void openMarketDetails(EveType::IdType typeId, Character::IdType charId, const ErrorCallback &errorCallback) const;

        void setDestination(quint64 locationId, Character::IdType charId, const ErrorCallback &errorCallback) const;

        void setEndpoints(EndpointMap endpoints);

        static void setRateLimit(float rate);

    public slots:
        void updateTokenAndContinue(Character::IdType charId, QString token, const QDateTime &expiry);
        void handleTokenError(const QString &error);

    signals:
        void tokenRequested(Character::IdType charId) const;

    private slots:
        void processSslErrors(const QList<QSslError> &errors);
        void processPendingRequests();

    private:
        using RegionUrlCallbackMap = QHash<QPair<uint, QString>, std::vector<std::function<void (const QUrl &, const QString &)>>>;
        using RegionUrlMap = QHash<uint, QUrl>;

        struct AccessToken
        {
            QString mToken;
            QDateTime mExpiry;
        };

        static const QString regionsUrlName;
        static const QString itemTypesUrlName;
        static const QString systemsUrlName;

        static RateLimiter mCRESTLimiter;
        static QTimer mRequestTimer;

        mutable QNetworkAccessManager mNetworkManager;
        EndpointMap mEndpoints;

        mutable RegionUrlMap mRegionUrls, mRegionOrdersUrls, mRegionHistoryUrls, mRegionMarketUrls;
        mutable RegionUrlCallbackMap mPendingRegionOrdersRequests, mPendingRegionHistoryRequests, mPendingRegionMarketRequests;

        mutable std::multimap<std::chrono::steady_clock::time_point, std::function<void ()>> mPendingRequests;
        mutable std::unordered_multimap<Character::IdType, std::function<void (const QString &)>> mPendingAuthRequests;

        mutable std::unordered_map<Character::IdType, AccessToken> mAccessTokens;

        template<class T>
        void checkAuth(Character::IdType charId, T &&continuation) const;

        template<class T>
        void fetchAccessToken(const T &continuation) const;

        template<class T>
        void getRegionUrl(uint regionId, RegionUrlMap &urlMap, RegionUrlCallbackMap &callbackMap, const QString &urlName, T &&continuation) const;

        template<class T>
        void getRegionData(QUrl regionUrl, EveType::IdType typeId, const QByteArray &accept, T &&continuation) const;
        template<class T>
        void getOrders(QUrl regionUrl, T &&continuation) const;

        void fetchPaginatedOrders(const PaginatedCallback &callback, const QUrl &url) const;

        template<class T>
        void asyncGet(const QUrl &url, const QByteArray &accept, T &&continuation) const;
        template<class T>
        void post(Character::IdType charId, const QUrl &url, const QByteArray &data, T &&errorCallback) const;

        template<class T>
        void scheduleRequest(T &&request) const;

        template<class T>
        void tryAuthAndContinue(Character::IdType charId, T &&continuation) const;

        QNetworkRequest prepareRequest(const QUrl &url, const QByteArray &accept) const;
        QNetworkRequest prepareRequest(Character::IdType charId, const QUrl &url, const QByteArray &accept) const;
    };
}
