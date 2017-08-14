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
#include "AssetList.h"
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
        using ExternalOrderList = std::vector<ExternalOrder>;
        using MarketOrderCallback = Callback<ExternalOrderList>;
        using HistoryMap = std::map<QDate, MarketHistoryEntry>;

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
                               const MarketOrderCallback &callback) const;
        void fetchMarketHistory(uint regionId,
                                EveType::IdType typeId,
                                const Callback<HistoryMap> &callback) const;
        void fetchMarketOrders(uint regionId, const MarketOrderCallback &callback) const;
        void fetchCitadelMarketOrders(quint64 citadelId,
                                      uint regionId,
                                      Character::IdType charId,
                                      const MarketOrderCallback &callback) const;
        void fetchAssets(Character::IdType charId, const Callback<AssetList> &callback) const;
        void fetchCharacter(Character::IdType charId, const Callback<Character> &callback) const;

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

        void handleNewPreferences();

    private:
        static const QString loginUrl;
        static const QString redirectDomain;
        static const QString firstTimeCitadelOrderImportKey;

        static std::unordered_map<Character::IdType, QString> mRefreshTokens;
        static bool mFetchingToken;

        static bool mFirstTimeCitadelOrderImport;

        const EveDataProvider &mDataProvider;
        const CharacterRepository &mCharacterRepo;

        const QByteArray mClientId;
        const QByteArray mClientSecret;

        SimpleCrypt mCrypt;

        std::vector<ESIInterface *> mInterfaces;
        mutable std::size_t mCurrentInterface = 0;

        QNetworkAccessManager mNetworkManager;

        std::unique_ptr<SSOAuthWidget> mAuthView;

        void processAuthorizationCode(Character::IdType charId, const QByteArray &code);

        QNetworkRequest getAuthRequest() const;

        ExternalOrder getOrderFromJson(const QJsonObject &object, uint regionId) const;
        ESIInterface::PaginatedCallback getMarketOrderCallback(uint regionId, const MarketOrderCallback &callback) const;

        void createInterfaces();

        const ESIInterface &selectNextInterface() const;

        static QNetworkRequest getVerifyRequest(const QByteArray &accessToken);
    };
}
