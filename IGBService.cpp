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
#include <limits>

#include <QSettings>

#include "MarketOrderProvider.h"
#include "EveDataProvider.h"
#include "IGBSettings.h"

#include "qxthttpsessionmanager.h"
#include "qxtwebevent.h"

#include "IGBService.h"

namespace Evernus
{
    IGBService::IGBService(const MarketOrderProvider &orderProvider,
                           const MarketOrderProvider &corpOrderProvider,
                           const EveDataProvider &dataProvider,
                           QxtHttpSessionManager *sm,
                           QObject *parent)
        : QxtWebSlotService(sm, parent)
        , mOrderProvider(orderProvider)
        , mCorpOrderProvider(corpOrderProvider)
        , mDataProvider(dataProvider)
    {
        mMainTemplate.open(":/html/igb_template.html");
        mOrderTemplate.open(":/html/igb_order_template.html");

        QSettings settings;

        mMainTemplate["active-link-text"] = tr("Character Active Orders");
        mMainTemplate["fulfilled-link-text"] = tr("Character Fulfilled Orders");
        mMainTemplate["corp-active-link-text"] = tr("Corporation Active Orders");
        mMainTemplate["corp-fulfilled-link-text"] = tr("Corporation Fulfilled Orders");
        mMainTemplate["open-margin-tool-link-text"] = tr("Open Margin Tool");
        mMainTemplate["port"] = settings.value(IGBSettings::portKey, IGBSettings::portDefault).toString();

        mOrderTemplate["sell-orders-text"] = tr("Sell orders:");
        mOrderTemplate["buy-orders-text"] = tr("Buy orders:");
        mOrderTemplate["prev-order-text"] = tr("Show Previous Order");
        mOrderTemplate["next-order-text"] = tr("Show Next Order");
        mOrderTemplate["current-order-text"] = tr("Current order:");
        mOrderTemplate["scan-delay-text"] = tr("Scan delay:");
        mOrderTemplate["start-scan-text"] = tr("Start Scan");
        mOrderTemplate["stop-scan-text"] = tr("Stop Scan");
        mOrderTemplate["stop-at-end-text"] = tr("Stop at end");
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
        showOrders<uint, &MarketOrderProvider::getSellOrdersForCorporation, &MarketOrderProvider::getBuyOrdersForCorporation>(
            event, mCorpOrderProvider, MarketOrder::State::Active, false, getCorporationId(event));
    }

    void IGBService::corpFulfilled(QxtWebRequestEvent *event)
    {
        showOrders<uint, &MarketOrderProvider::getSellOrdersForCorporation, &MarketOrderProvider::getBuyOrdersForCorporation>(
            event, mCorpOrderProvider, MarketOrder::State::Fulfilled, true, getCorporationId(event));
    }

    void IGBService::openMarginTool(QxtWebRequestEvent *event)
    {
        Q_UNUSED(event);
        emit openMarginTool();
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
                                        QStringList &typeIdContainer) const
    {
        QString result;
        for (const auto &order : orders)
        {
            idContainer << QString::number(order->getId());
            typeIdContainer << QString::number(order->getTypeId());

            result.append(QString{"<li><a class='order' id='order-%3' href='#' onclick='showOrder(%1);'>%2</a></li>"}
                .arg(order->getId())
                .arg(mDataProvider.getTypeName(order->getTypeId()))
                .arg(order->getId()));
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

        mOrderTemplate["sell-orders"] = renderOrderList(
            filterAndSort((provider.*SellFunc)(id), state, needsDelta), idContainer, typeIdContainer);
        mOrderTemplate["buy-orders-start"] = QString::number(idContainer.size() + 1);
        mOrderTemplate["buy-orders"] = renderOrderList(
            filterAndSort((provider.*BuyFunc)(id), state, needsDelta), idContainer, typeIdContainer);
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
}
