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

#include <vector>
#include <memory>

#include "MarketOrder.h"

#include "qxtwebslotservice.h"
#include "qxthtmltemplate.h"

#include "Character.h"

class QxtHttpSessionManager;
class QWebSocket;

namespace Evernus
{
    class FavoriteItemRepository;
    class MarketOrderProvider;
    class CharacterRepository;
    class ItemCostProvider;
    class EveDataProvider;

    class IGBService
        : public QxtWebSlotService
    {
        Q_OBJECT

    public:
        IGBService(const MarketOrderProvider &orderProvider,
                   const MarketOrderProvider &corpOrderProvider,
                   const EveDataProvider &dataProvider,
                   const ItemCostProvider &itemCostProvider,
                   const FavoriteItemRepository &favoriteItemRepo,
                   const CharacterRepository &characterRepository,
                   QxtHttpSessionManager *sm,
                   QObject *parent = nullptr);
        virtual ~IGBService() = default;

    signals:
        void openMarginTool();

    public slots:
        void index(QxtWebRequestEvent *event);
        void active(QxtWebRequestEvent *event);
        void fulfilled(QxtWebRequestEvent *event);
        void overbid(QxtWebRequestEvent *event);
        void belowMargin(QxtWebRequestEvent *event);
        void corpActive(QxtWebRequestEvent *event);
        void corpFulfilled(QxtWebRequestEvent *event);
        void corpOverbid(QxtWebRequestEvent *event);
        void corpBelowMargin(QxtWebRequestEvent *event);
        void favorite(QxtWebRequestEvent *event);
        void openMarginTool(QxtWebRequestEvent *event);
        void update(QxtWebRequestEvent *event);

        void setDestinationInEve(quint64 locationId);

    private:
        typedef std::vector<std::shared_ptr<MarketOrder>> OrderList;

        static const QString limitToStationsCookie;
        static const QString orderHtmlTemplate;
        static const QString setDestinationMessage;

        const MarketOrderProvider &mOrderProvider, &mCorpOrderProvider;
        const EveDataProvider &mDataProvider;
        const ItemCostProvider &mItemCostProvider;

        const FavoriteItemRepository &mFavoriteItemRepo;
        const CharacterRepository &mCharacterRepository;

        QxtHtmlTemplate mMainTemplate, mOrderTemplate, mFavoriteTemplate;

        std::vector<QxtWebRequestEvent *> mPollingRequests;

        void renderContent(QxtWebRequestEvent *event, const QString &content);
        QString renderOrderList(const std::vector<std::shared_ptr<MarketOrder>> &orders,
                                QStringList &idContainer,
                                QStringList &typeIdContainer,
                                uint stationId) const;

        template<class SellFunc, class BuyFunc>
        void showOrders(QxtWebRequestEvent *event,
                        const SellFunc &sellFunc,
                        const BuyFunc &buyFunc,
                        MarketOrder::State state,
                        bool needsDelta);
        OrderList filterAndSort(const OrderList &orders, MarketOrder::State state, bool needsDelta) const;

        uint getStationIdForFiltering(QxtWebRequestEvent *event) const;

        void sendEventToIGB(QByteArray message);

        static Character::IdType getCharacterId(QxtWebRequestEvent *event);
        static uint getCorporationId(QxtWebRequestEvent *event);
        static uint getStationId(QxtWebRequestEvent *event);
    };
}
