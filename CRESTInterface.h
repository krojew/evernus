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

        using QObject::QObject;
        virtual ~CRESTInterface() = default;

        void fetchBuyMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const;
        void fetchSellMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const;

    public slots:
        void updateTokenAndContinue(QString token, const QDateTime &expiry);
        void handleTokenError(const QString &error);

    signals:
        void tokenRequested() const;

    private:
        using RegionOrderUrlMap = QHash<uint, QUrl>;

        static const QString crestUrl;

        static const QString regionsUrlName;
        static const QString itemTypesUrlName;

        mutable QNetworkAccessManager mNetworkManager;
        mutable QHash<QString, QString> mEndpoints;

        mutable QString mAccessToken;
        mutable QDateTime mExpiry;

        mutable std::vector<std::function<void (const QString &)>> mPendingRequests;

        mutable RegionOrderUrlMap mRegionBuyOrdersUrls, mRegionSellOrdersUrls;

        QUrl getRegionBuyOrdersUrl(uint regionId) const;

        QUrl getRegionSellOrdersUrl(uint regionId) const;

        QUrl getRegionOrdersUrl(uint regionId,
                                const QString &urlName,
                                RegionOrderUrlMap &map) const;

        QJsonDocument getOrders(QUrl regionUrl, EveType::IdType typeId) const;

        template<class T>
        void checkAuth(const T &continuation) const;

        template<class T>
        void fetchAccessToken(const T &continuation) const;

        QJsonDocument syncGet(const QUrl &url, const QByteArray &accept) const;
    };
}
