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
                           const EveDataProvider &dataProvider,
                           QxtHttpSessionManager *sm,
                           QObject *parent)
        : QxtWebSlotService(sm, parent)
        , mOrderProvider(orderProvider)
        , mDataProvider(dataProvider)
    {
        mMainTemplate.open(":/html/igb_template.html");
        mOrderTemplate.open(":/html/order_template.html");

        QSettings settings;

        mMainTemplate["active-link-text"] = tr("Active Orders");
        mMainTemplate["fulfilled-link-text"] = tr("Fulfilled Orders");
        mMainTemplate["open-margin-tool-link-text"] = tr("Open Margin Tool");
        mMainTemplate["port"] = settings.value(IGBSettings::portKey, IGBSettings::portDefault).toString();

        mOrderTemplate["sell-orders-text"] = tr("Sell orders:");
        mOrderTemplate["buy-orders-text"] = tr("Buy orders:");
        mOrderTemplate["prev-order-text"] = tr("Show Previous Order");
        mOrderTemplate["next-order-text"] = tr("Show Next Order");
        mOrderTemplate["current-order-text"] = tr("Current order:");
    }

    void IGBService::index(QxtWebRequestEvent *event)
    {
        renderContent(event, tr("Please select a category from the menu."));
    }

    void IGBService::active(QxtWebRequestEvent *event)
    {
        QStringList idContainer, typeIdContainer;

        mOrderTemplate["sell-orders"] = renderOrderList(
            filterAndSort(mOrderProvider.getSellOrders(getCharacterId(event)), MarketOrder::State::Active, false), idContainer, typeIdContainer);
        mOrderTemplate["buy-orders-start"] = QString::number(idContainer.size() + 1);
        mOrderTemplate["buy-orders"] = renderOrderList(
            filterAndSort(mOrderProvider.getBuyOrders(getCharacterId(event)), MarketOrder::State::Active, false), idContainer, typeIdContainer);
        mOrderTemplate["order-ids"] = idContainer.join(", ");
        mOrderTemplate["type-ids"] = typeIdContainer.join(", ");

        renderContent(event, mOrderTemplate.render());
    }

    void IGBService::fulfilled(QxtWebRequestEvent *event)
    {
        QStringList idContainer, typeIdContainer;

        mOrderTemplate["sell-orders"] = renderOrderList(
            filterAndSort(mOrderProvider.getSellOrders(getCharacterId(event)), MarketOrder::State::Fulfilled, true), idContainer, typeIdContainer);
        mOrderTemplate["buy-orders-start"] = QString::number(idContainer.size() + 1);
        mOrderTemplate["buy-orders"] = renderOrderList(
            filterAndSort(mOrderProvider.getBuyOrders(getCharacterId(event)), MarketOrder::State::Fulfilled, true), idContainer, typeIdContainer);
        mOrderTemplate["order-ids"] = idContainer.join(", ");
        mOrderTemplate["type-ids"] = typeIdContainer.join(", ");

        renderContent(event, mOrderTemplate.render());
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

            result.append(QString{"<li><a class='order' id='order-%3' href='#' onclick='showType(%1);'>%2</a></li>"}
                .arg(order->getTypeId())
                .arg(mDataProvider.getTypeName(order->getTypeId()))
                .arg(order->getId()));
        }

        return result;
    }

    Character::IdType IGBService::getCharacterId(QxtWebRequestEvent *event)
    {
        const auto charIdHeader = "EVE_CHARID";

        if (!event->headers.contains(charIdHeader))
            return Character::invalidId;

        return event->headers.values(charIdHeader).first().toULongLong();
    }

    std::vector<std::shared_ptr<MarketOrder>> IGBService
    ::filterAndSort(const std::vector<std::shared_ptr<MarketOrder>> &orders, MarketOrder::State state, bool needsDelta) const
    {
        std::vector<std::shared_ptr<MarketOrder>> result;
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
}
