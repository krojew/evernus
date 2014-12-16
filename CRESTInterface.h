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
#include <memory>
#include <vector>

#include <QNetworkAccessManager>
#include <QDateTime>
#include <QWebView>
#include <QHash>

#include "SimpleCrypt.h"
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

        CRESTInterface(QByteArray clientId, QByteArray clientSecret, QObject *parent = nullptr);
        virtual ~CRESTInterface() = default;

        bool hasClientCredentials() const;

        virtual bool eventFilter(QObject *watched, QEvent *event) override;

        void fetchBuyMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const;
        void fetchSellMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const;

    private:
        using RegionOrderUrlMap = QHash<uint, QUrl>;

        static const QString crestUrl;
        static const QString loginUrl;
        static const QString redirectUrl;

        static const QString regionsUrlName;
        static const QString itemTypesUrlName;

        const QByteArray mClientId;
        const QByteArray mClientSecret;

        mutable SimpleCrypt mCrypt;

        mutable QNetworkAccessManager mNetworkManager;
        mutable QHash<QString, QString> mEndpoints;

        mutable QString mRefreshToken, mAccessToken;
        mutable QDateTime mExpiry;

        mutable std::vector<std::function<void (const QString &)>> mPendingRequests;

        mutable std::unique_ptr<QWebView> mAuthView;

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
