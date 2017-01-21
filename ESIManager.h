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
#include <vector>
#include <memory>
#include <map>

#include <QNetworkAccessManager>
#include <QString>
#include <QDate>

#include "MarketHistoryEntry.h"
#include "SSOAuthWidget.h"
#include "ESIInterface.h"
#include "SimpleCrypt.h"
#include "Character.h"
#include "EveType.h"

class QJsonObject;

namespace Evernus
{
    class CharacterRepository;
    class EveDataProvider;
    class ExternalOrder;

    class ESIManager final
        : public QObject
    {
        Q_OBJECT

    public:
        template<class T>
        using Callback = std::function<void (T &&data, const QString &error)>;

        ESIManager(QByteArray clientId,
                   QByteArray clientSecret,
                   const EveDataProvider &dataProvider,
                   const CharacterRepository &characterRepo,
                   QObject *parent = nullptr);
        ESIManager(const ESIManager &) = default;
        ESIManager(ESIManager &&) = default;
        virtual ~ESIManager() = default;

        virtual bool eventFilter(QObject *watched, QEvent *event) override;

        void fetchMarketOrders(uint regionId,
                               EveType::IdType typeId,
                               const Callback<std::vector<ExternalOrder>> &callback) const;
        void fetchMarketHistory(uint regionId,
                                EveType::IdType typeId,
                                const Callback<std::map<QDate, MarketHistoryEntry>> &callback) const;
        void fetchMarketOrders(uint regionId, const Callback<std::vector<ExternalOrder>> &callback) const;

        void openMarketDetails(EveType::IdType typeId, Character::IdType charId) const;

        void setDestination(quint64 locationId, Character::IdType charId) const;

        bool hasClientCredentials() const;

        ESIManager &operator =(const ESIManager &) = default;
        ESIManager &operator =(ESIManager &&) = default;

    signals:
        void error(const QString &text) const;

        void tokenError(const QString &error);
        void acquiredToken(Character::IdType charId, const QString &accessToken, const QDateTime &expiry);

    public slots:
        void fetchToken(Character::IdType charId);

    private:
        static const QString loginUrl;
        static const QString redirectDomain;

        static std::unordered_map<Character::IdType, QString> mRefreshTokens;
        static bool mFetchingToken;

        const EveDataProvider &mDataProvider;
        const CharacterRepository &mCharacterRepo;

        const QByteArray mClientId;
        const QByteArray mClientSecret;

        SimpleCrypt mCrypt;

        ESIInterface mInterface;

        QNetworkAccessManager mNetworkManager;

        std::unique_ptr<SOOAuthWidget> mAuthView;

        void processAuthorizationCode(Character::IdType charId, const QByteArray &code);

        QNetworkRequest getAuthRequest() const;

        ExternalOrder getOrderFromJson(const QJsonObject &object, uint regionId) const;

        static QNetworkRequest getVerifyRequest(const QByteArray &accessToken);
    };
}
