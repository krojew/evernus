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
#include <functional>

#include <QNetworkAccessManager>
#include <QSettings>
#include <QDateTime>
#include <QString>

#include "Character.h"
#include "EveType.h"

class QNetworkRequest;
class QJsonDocument;

namespace Evernus
{
    class ESIInterface final
        : public QObject
    {
        Q_OBJECT

    public:
        using JsonCallback = std::function<void (QJsonDocument &&data, const QString &error)>;
        using PaginatedCallback = std::function<void (QJsonDocument &&data, bool atEnd, const QString &error)>;
        using ErrorCallback = std::function<void (const QString &error)>;

        using QObject::QObject;
        ESIInterface() = default;
        ESIInterface(const ESIInterface &) = default;
        ESIInterface(ESIInterface &&) = default;
        virtual ~ESIInterface() = default;

        void fetchMarketOrders(uint regionId, EveType::IdType typeId, const JsonCallback &callback) const;
        void fetchMarketOrders(uint regionId, const PaginatedCallback &callback) const;
        void fetchMarketHistory(uint regionId, EveType::IdType typeId, const JsonCallback &callback) const;

        void fetchCitadelMarketOrders(quint64 citadelId, Character::IdType charId, const PaginatedCallback &callback) const;

        void openMarketDetails(EveType::IdType typeId, Character::IdType charId, const ErrorCallback &errorCallback) const;

        void setDestination(quint64 locationId, Character::IdType charId, const ErrorCallback &errorCallback) const;

        ESIInterface &operator =(const ESIInterface &) = default;
        ESIInterface &operator =(ESIInterface &&) = default;

    public slots:
        void updateTokenAndContinue(Character::IdType charId, QString token, const QDateTime &expiry);
        void handleTokenError(const QString &error);

    signals:
        void tokenRequested(Character::IdType charId) const;

    private slots:
        void processSslErrors(const QList<QSslError> &errors);

    private:
        struct AccessToken
        {
            QString mToken;
            QDateTime mExpiry;
        };

        static const QString esiUrl;

        mutable QNetworkAccessManager mNetworkManager;

        mutable std::unordered_multimap<Character::IdType, std::function<void (const QString &)>> mPendingAuthRequests;
        mutable std::unordered_map<Character::IdType, AccessToken> mAccessTokens;

        QSettings mSettings;

        template<class T>
        void checkAuth(Character::IdType charId, T &&continuation) const;

        template<class T>
        void fetchPaginatedData(const QString &url, uint page, T &&continuation) const;
        template<class T>
        void fetchPaginatedData(Character::IdType charId, const QString &url, uint page, T &&continuation, bool suppressForbidden = false) const;

        template<class T>
        void asyncGet(const QString &url, const QString &query, T &&continuation, uint retries) const;
        template<class T>
        void asyncGet(Character::IdType charId,
                      const QString &url,
                      const QString &query,
                      T &&continuation,
                      uint retries,
                      bool suppressForbidden = false) const;
        template<class T>
        void post(Character::IdType charId, const QString &url, const QString &query, T &&errorCallback) const;

        template<class T>
        void tryAuthAndContinue(Character::IdType charId, T &&continuation) const;

        QNetworkRequest prepareRequest(const QString &url, const QString &query) const;
        QNetworkRequest prepareRequest(Character::IdType charId, const QString &url, const QString &query) const;

        uint getNumRetries() const;

        static QString getError(const QString &url, const QString &query, QNetworkReply &reply);
    };
}
