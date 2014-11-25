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

#include <functional>
#include <memory>
#include <vector>

#include <QNetworkAccessManager>
#include <QDateTime>
#include <QWebView>
#include <QHash>

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

        virtual bool eventFilter(QObject *watched, QEvent *event) override;

        void fetchMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const;

    private:
        static const QString crestUrl;
        static const QString loginUrl;
        static const QString redirectUrl;

        static const QString authUrlName;

        mutable QNetworkAccessManager mNetworkManager;
        mutable QHash<QString, QString> mEndpoints;

        mutable QString mRefreshToken, mAccessToken;
        mutable QDateTime mExpiry;

        mutable std::vector<std::function<void (const QString &)>> mPendingRequests;

        mutable std::unique_ptr<QWebView> mAuthView;

        template<class T>
        void checkAuth(const T &continuation) const;

        template<class T>
        void fetchAccessToken(const T &continuation) const;
    };
}
