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
#include <algorithm>
#include <stdexcept>
#include <limits>

#include <QSettings>
#include <QFile>

#include "FavoriteItemRepository.h"
#include "MarketOrderProvider.h"
#include "EveDataProvider.h"
#include "IGBSettings.h"

#include "qxthttpsessionmanager.h"
#include "qxtwebevent.h"

#include "IGBService.h"

namespace Evernus
{
    const QString IGBService::limitToStationsCookie = "limitToStations";
    const QString IGBService::orderHtmlTemplate = "<li><a class='order' id='order-%2' href='#' onclick='showOrder(%2);'>%1</a></li>";

    IGBService::IGBService(const MarketOrderProvider &orderProvider,
                           const MarketOrderProvider &corpOrderProvider,
                           const EveDataProvider &dataProvider,
                           const FavoriteItemRepository &favoriteItemRepo,
                           QxtHttpSessionManager *sm,
                           QObject *parent)
        : QxtWebSlotService(sm, parent)
        , mOrderProvider(orderProvider)
        , mCorpOrderProvider(corpOrderProvider)
        , mDataProvider(dataProvider)
        , mFavoriteItemRepo(favoriteItemRepo)
    {
        mMainTemplate.open(":/html/igb_template.html");
        mOrderTemplate.open(":/html/igb_order_template.html");
        mFavoriteTemplate.open(":/html/igb_favorite_template.html");

        QFile orderScript{":/html/igb_order_js_template.html"};
        if (!orderScript.open(QIODevice::ReadOnly))
            throw std::runtime_error{tr("Error reading IGB script template.").toStdString()};

        QSettings settings;

        mMainTemplate["active-link-text"] = tr("Character Active Orders");
        mMainTemplate["fulfilled-link-text"] = tr("Character Fulfilled Orders");
        mMainTemplate["corp-active-link-text"] = tr("Corporation Active Orders");
        mMainTemplate["corp-fulfilled-link-text"] = tr("Corporation Fulfilled Orders");
        mMainTemplate["favorite-link-text"] = tr("Favorite Items");
        mMainTemplate["open-margin-tool-link-text"] = tr("Open Margin Tool");
        mMainTemplate["port"] = settings.value(IGBSettings::portKey, IGBSettings::portDefault).toString();

        mOrderTemplate["order-script"] = orderScript.readAll();
        mOrderTemplate["prev-order-text"] = tr("Show Previous Entry");
        mOrderTemplate["next-order-text"] = tr("Show Next Entry");
        mOrderTemplate["current-order-text"] = tr("Current entry:");
        mOrderTemplate["scan-delay-text"] = tr("Scan delay:");
        mOrderTemplate["start-scan-text"] = tr("Start Scan");
        mOrderTemplate["stop-scan-text"] = tr("Stop Scan");
        mOrderTemplate["stop-at-end-text"] = tr("Stop at end");

        mFavoriteTemplate.copyArguments(mOrderTemplate);
        mFavoriteTemplate["favorite-text"] = tr("Favorite items:");

        mOrderTemplate["sell-orders-text"] = tr("Sell orders:");
        mOrderTemplate["buy-orders-text"] = tr("Buy orders:");
        mOrderTemplate["limit-to-stations-text"] = tr("Limit to current station, if available");
        mOrderTemplate["limit-to-stations-cookie"] = limitToStationsCookie;
    }

    void IGBService::index(QxtWebRequestEvent *event)
    {
        renderContent(event, tr("Please select a category from the menu."));
    }

    void IGBService::active(QxtWebRequestEvent *event)
    {
        showOrders<Character::IdType, &MarketOrderProvider::getSellOrders, &MarketOrderProvider::getBuyOrders>(
            event, mOrderProvider, MarketOrder::State::Active, false, getCharacterId(event));
    }

    void IGBService::fulfilled(QxtWebRequestEvent *event)
    {
        showOrders<Character::IdType, &MarketOrderProvider::getSellOrders, &MarketOrderProvider::getBuyOrders>(
            event, mOrderProvider, MarketOrder::State::Fulfilled, true, getCharacterId(event));
    }

    void IGBService::corpActive(QxtWebRequestEvent *event)
    {
        showOrders<quint64, &MarketOrderProvider::getSellOrdersForCorporation, &MarketOrderProvider::getBuyOrdersForCorporation>(
            event, mCorpOrderProvider, MarketOrder::State::Active, false, getCorporationId(event));
    }

    void IGBService::corpFulfilled(QxtWebRequestEvent *event)
    {
        showOrders<quint64, &MarketOrderProvider::getSellOrdersForCorporation, &MarketOrderProvider::getBuyOrdersForCorporation>(
            event, mCorpOrderProvider, MarketOrder::State::Fulfilled, true, getCorporationId(event));
    }

    void IGBService::favorite(QxtWebRequestEvent *event)
    {
        QStringList idContainer, typeIdContainer;
        QString result;

        const auto items = mFavoriteItemRepo.fetchAll();
        auto i = 0u;

        for (const auto &item : items)
        {
            idContainer << QString::number(++i);
            typeIdContainer << QString::number(item->getId());

            result.append(orderHtmlTemplate.arg(mDataProvider.getTypeName(item->getId())).arg(idContainer.last()));
        }

        QSettings settings;

        mFavoriteTemplate["order-ids"] = idContainer.join(", ");
        mFavoriteTemplate["type-ids"] = typeIdContainer.join(", ");
        mFavoriteTemplate["scan-delay"] = settings.value(IGBSettings::scanDelayKey, IGBSettings::scanDelayDefault).toString();
        mFavoriteTemplate["favorite-items"] = result;

        renderContent(event, mFavoriteTemplate.render());
    }

    void IGBService::openMarginTool(QxtWebRequestEvent *event)
    {
        emit openMarginTool();

        postEvent(new QxtWebPageEvent(event->sessionID, event->requestID, QByteArray{}));
    }

    void IGBService::renderContent(QxtWebRequestEvent *event, const QString &content)
    {
        const auto trustedHeader = "EVE_TRUSTED";

        mMainTemplate["content"] = (!event->headers.contains(trustedHeader) || event->headers.values(trustedHeader).first() != "Yes") ?
                                   (QString{"<h3>%1</h3>%2"}.arg(tr("Website trust is required. Please add it to Trusted Sites.")).arg(content)) :
                                   (content);

        postEvent(new QxtWebPageEvent(event->sessionID, event->requestID, mMainTemplate.render().toUtf8()));
    }

    QString IGBService::renderOrderList(const std::vector<std::shared_ptr<MarketOrder>> &orders,
                                        QStringList &idContainer,
                                        QStringList &typeIdContainer,
                                        uint stationId) const
    {
        QString result;
        for (const auto &order : orders)
        {
            if (stationId != 0 && stationId != order->getStationId())
                continue;

            idContainer << QString::number(order->getId());
            typeIdContainer << QString::number(order->getTypeId());

            result.append(orderHtmlTemplate.arg(mDataProvider.getTypeName(order->getTypeId())).arg(order->getId()));
        }

        return result;
    }

    template<class T, IGBService::OrderList (MarketOrderProvider::* SellFunc)(T) const, IGBService::OrderList (MarketOrderProvider::* BuyFunc)(T) const>
    void IGBService::showOrders(QxtWebRequestEvent *event,
                                const MarketOrderProvider &provider,
                                MarketOrder::State state,
                                bool needsDelta,
                                T id)
    {
        QStringList idContainer, typeIdContainer;
        QSettings settings;

        const auto stationId = getStationIdForFiltering(event);

        mOrderTemplate["sell-orders"] = renderOrderList(
            filterAndSort((provider.*SellFunc)(id), state, needsDelta), idContainer, typeIdContainer, stationId);
        mOrderTemplate["buy-orders-start"] = QString::number(idContainer.size() + 1);
        mOrderTemplate["buy-orders"] = renderOrderList(
            filterAndSort((provider.*BuyFunc)(id), state, needsDelta), idContainer, typeIdContainer, stationId);
        mOrderTemplate["order-ids"] = idContainer.join(", ");
        mOrderTemplate["type-ids"] = typeIdContainer.join(", ");
        mOrderTemplate["scan-delay"] = settings.value(IGBSettings::scanDelayKey, IGBSettings::scanDelayDefault).toString();

        renderContent(event, mOrderTemplate.render());
    }

    IGBService::OrderList IGBService::filterAndSort(const OrderList &orders, MarketOrder::State state, bool needsDelta) const
    {
        OrderList result;
        result.reserve(orders.size());

        for (const auto &order : orders)
        {
            if (order->getState() == state && (!needsDelta || order->getDelta() != 0))
                result.emplace_back(order);
        }

        std::sort(std::begin(result), std::end(result), [this](const auto &o1, const auto &o2) {
            return mDataProvider.getTypeName(o1->getTypeId()) < mDataProvider.getTypeName(o2->getTypeId());
        });
        return result;
    }

    uint IGBService::getStationIdForFiltering(QxtWebRequestEvent *event) const
    {
        if (!event->cookies.contains(limitToStationsCookie))
            return 0;

        const auto value = event->cookies.values(limitToStationsCookie).first();
        return (value.compare("false", Qt::CaseInsensitive) == 0) ? (0) : (getStationId(event));
    }

    Character::IdType IGBService::getCharacterId(QxtWebRequestEvent *event)
    {
        const auto charIdHeader = "EVE_CHARID";

        if (!event->headers.contains(charIdHeader))
            return Character::invalidId;

        return event->headers.values(charIdHeader).first().toULongLong();
    }

    uint IGBService::getCorporationId(QxtWebRequestEvent *event)
    {
        const auto corpIdHeader = "EVE_CORPID";

        if (!event->headers.contains(corpIdHeader))
            return std::numeric_limits<uint>::max();    // 0 fetches orders without a corp

        return event->headers.values(corpIdHeader).first().toUInt();
    }

    uint IGBService::getStationId(QxtWebRequestEvent *event)
    {
        const auto stationIdHeader = "EVE_STATIONID";

        if (!event->headers.contains(stationIdHeader))
            return 0;

        return event->headers.values(stationIdHeader).first().toUInt();
    }
}
