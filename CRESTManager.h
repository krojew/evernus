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
#include <vector>
#include <memory>
#include <map>

#include <QDate>

#include "MarketHistoryEntry.h"
#include "CRESTAuthWidget.h"
#include "CRESTInterface.h"
#include "SimpleCrypt.h"
#include "EveType.h"

namespace Evernus
{
    class EveDataProvider;
    class ExternalOrder;

    class CRESTManager
        : public QObject
    {
        Q_OBJECT

    public:
        template<class T>
        using Callback = std::function<void (T &&data, const QString &error)>;

        CRESTManager(QByteArray clientId,
                     QByteArray clientSecret,
                     const EveDataProvider &dataProvider,
                     QObject *parent = nullptr);
        virtual ~CRESTManager() = default;

        virtual bool eventFilter(QObject *watched, QEvent *event) override;

        void fetchMarketOrders(uint regionId,
                               EveType::IdType typeId,
                               const Callback<std::vector<ExternalOrder>> &callback) const;
        void fetchMarketHistory(uint regionId,
                                EveType::IdType typeId,
                                const Callback<std::map<QDate, MarketHistoryEntry>> &callback) const;

        bool hasClientCredentials() const;

    signals:
        void tokenError(const QString &error);
        void acquiredToken(const QString &accessToken, const QDateTime &expiry);

    public slots:
        void fetchToken();

        void handleNewPreferences();

    private slots:
        void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

    private:
        static const QString loginUrl;
        static const QString redirectDomain;

        const EveDataProvider &mDataProvider;

        const QByteArray mClientId;
        const QByteArray mClientSecret;

        SimpleCrypt mCrypt;

        std::vector<CRESTInterface *> mInterfaces;
        mutable size_t mCurrentInterface = 0;

        QString mRefreshToken;

        QNetworkAccessManager mNetworkManager;

        std::unique_ptr<CRESTAuthWidget> mAuthView;

        bool mFetchingToken = false;

        void processAuthorizationCode(const QByteArray &code);

        QNetworkRequest getAuthRequest() const;

        const CRESTInterface &selectNextInterface() const;
    };
}
